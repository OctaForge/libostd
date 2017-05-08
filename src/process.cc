/* Process handling implementation bits.
 *
 * This file is part of libostd. See COPYING.md for futher information.
 */

#include <cstddef>
#include <cstdlib>
#include <cerrno>
#include <string>
#include <memory>
#include <new>

#include "ostd/process.hh"

#ifdef OSTD_PLATFORM_WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <unistd.h>
#  include <fcntl.h>
#  include <sys/wait.h>
#  include <wordexp.h>
#endif

namespace ostd {
namespace detail {

OSTD_EXPORT void split_args_impl(
    string_range const &str, void (*func)(string_range, void *), void *data
) {
#ifndef OSTD_PLATFORM_WIN32
    std::string strs{str};

    wordexp_t p;
    if (int err; (err = wordexp(strs.data(), &p, 0))) {
        switch (err) {
        case WRDE_BADCHAR:
            throw word_error{"illegal character"};
        case WRDE_BADVAL:
            /* no WRDE_UNDEF flag, won't happen */
            throw word_error{"undefined shell variable"};
        case WRDE_CMDSUB:
            /* no WRDE_NOCMD flag, won't happen */
            throw word_error{"invalid command substitution"};
        case WRDE_NOSPACE:
            throw std::bad_alloc{};
        case WRDE_SYNTAX:
            throw word_error{"syntax error"};
        default:
            throw word_error{"unknown error"};
        }
    }

    /* if the output range throws, make sure stuff gets freed */
    struct wordexp_guard {
        void operator()(wordexp_t *fp) const {
            wordfree(fp);
        }
    };
    std::unique_ptr<wordexp_t, wordexp_guard> guard{&p};

    for (std::size_t i = 0; i < p.we_wordc; ++i) {
        func(p.we_wordv[i], data);
    }
#else
    std::unique_ptr<wchar_t[]> wstr{new wchar_t[str.size() + 1]};
    memset(wstr.get(), 0, (str.size() + 1) * sizeof(wchar_t));

    if (!MultiByteToWideChar(
        CP_UTF8, 0, str.data(), str.size(), wstr.get(), str.size() + 1
    )) {
        switch (GetLastError()) {
        case ERROR_NO_UNICODE_TRANSLATION:
            throw word_error{"unicode conversion failed"};
        default:
            throw word_error{"unknown error"};
        }
    }

    int argc = 0;
    wchar_t **pwargs = CommandLineToArgvW(wstr.get(), &argc);

    if (!pwargs) {
        throw word_error{"command line parsing failed"};
    }

    /* if anything throws, make sure stuff gets freed */
    struct wchar_guard {
        void operator()(wchar_t **p) const {
            LocalFree(p);
        }
    };
    std::unique_ptr<wchar_t *, wchar_guard> wguard{pwargs};

    for (int i = 0; i < argc; ++i) {
        wchar_t *arg = pwargs[i];
        std::size_t arglen = wcslen(arg);

        std::size_t req = 0;
        if (!(req = WideCharToMultiByte(
            CP_UTF8, 0, arg, arglen, nullptr, 0, nullptr, nullptr
        ))) {
            switch (GetLastError()) {
            case ERROR_NO_UNICODE_TRANSLATION:
                throw word_error{"unicode conversion failed"};
            default:
                throw word_error{"unknown error"};
            }
        }

        std::unique_ptr<char[]> buf{new char[req]};
        if (!WideCharToMultiByte(
            CP_UTF8, 0, arg, arglen, buf.get(), req, nullptr, nullptr
        )) {
            switch (GetLastError()) {
            case ERROR_NO_UNICODE_TRANSLATION:
                throw word_error{"unicode conversion failed"};
            default:
                throw word_error{"unknown error"};
            }
        }

        func(string_range{buf.get(), buf.get() + req - 1}, data);
    }
#endif
}

} /* namespace detail */

#ifndef OSTD_PLATFORM_WIN32
struct data {
    int pid = -1, errno_fd = -1;
};

struct pipe {
    int fd[2] = { -1, -1 };

    ~pipe() {
        if (fd[0] >= 0) {
            ::close(fd[0]);
        }
        if (fd[1] >= 0) {
            ::close(fd[1]);
        }
    }

    void open(process_stream use) {
        if (use != process_stream::PIPE) {
            return;
        }
        if (::pipe(fd) < 0) {
            throw process_error{errno, std::generic_category()};
        }
    }

    int &operator[](std::size_t idx) {
        return fd[idx];
    }

    void fdopen(file_stream &s, bool write) {
        FILE *p = ::fdopen(fd[std::size_t(write)], write ? "w" : "r");
        if (!p) {
            throw process_error{errno, std::generic_category()};
        }
        /* do not close twice, the stream will close it */
        fd[std::size_t(write)] = -1;
        s.open(p, [](FILE *f) {
            std::fclose(f);
        });
    }

    void close(bool write) {
        ::close(std::exchange(fd[std::size_t(write)], -1));
    }

    bool dup2(int target, pipe &err, bool write) {
        if (::dup2(fd[std::size_t(write)], target) < 0) {
            err.write_errno();
            return false;
        }
        close(write);
        return true;
    }

    void write_errno() {
        write(fd[1], int(errno), sizeof(int));
    }
};

OSTD_EXPORT void subprocess::open_impl(
    std::string const &cmd, std::vector<std::string> const &args,
    bool use_path
) {
    if (use_in == process_stream::STDOUT) {
        throw process_error{EINVAL, std::generic_category()};
    }

    auto argp = std::make_unique<char *[]>(args.size() + 1);
    for (std::size_t i = 0; i < args.size(); ++i) {
        argp[i] = const_cast<char *>(args[i].data());
    }
    argp[args.size()] = nullptr;

    p_current = ::new (reinterpret_cast<void *>(&p_data)) data{};
    data *pd = static_cast<data *>(p_current);

    /* fd_errno used to detect if exec failed */
    pipe fd_errno, fd_stdin, fd_stdout, fd_stderr;

    fd_errno.open(process_stream::PIPE);
    fd_stdin.open(use_in);
    fd_stdout.open(use_out);
    fd_stderr.open(use_err);

    auto cpid = fork();
    if (cpid == -1) {
        throw process_error{errno, std::generic_category()};
    } else if (!cpid) {
        /* child process */
        fd_errno.close(false);
        /* fcntl fails, write the errno to be read from parent */
        if (fcntl(fd_errno[1], F_SETFD, FD_CLOEXEC) < 0) {
            fd_errno.write_errno();
            std::exit(1);
        }
        /* prepare standard streams */
        if (use_in == process_stream::PIPE) {
            fd_stdin.close(true);
            if (!fd_stdin.dup2(STDIN_FILENO, fd_errno, false)) {
                std::exit(1);
            }
        }
        if (use_out == process_stream::PIPE) {
            fd_stdout.close(false);
            if (!fd_stdout.dup2(STDOUT_FILENO, fd_errno, true)) {
                std::exit(1);
            }
        }
        if (use_err == process_stream::PIPE) {
            fd_stderr.close(false);
            if (!fd_stderr.dup2(STDERR_FILENO, fd_errno, true)) {
                std::exit(1);
            }
        } else if (use_err == process_stream::STDOUT) {
            if (dup2(STDOUT_FILENO, STDERR_FILENO) < 0) {
                fd_errno.write_errno();
                std::exit(1);
            }
        }
        if (use_path) {
            execvp(cmd.data(), argp.get());
        } else {
            execv(cmd.data(), argp.get());
        }
        /* exec has returned, so error has occured */
        fd_errno.write_errno();
        std::exit(1);
    } else {
        /* parent process */
        fd_errno.close(true);
        if (use_in == process_stream::PIPE) {
            fd_stdin.close(false);
            fd_stdin.fdopen(in, true);
        }
        if (use_out == process_stream::PIPE) {
            fd_stdout.close(true);
            fd_stdout.fdopen(out, false);
        }
        if (use_err == process_stream::PIPE) {
            fd_stderr.close(true);
            fd_stderr.fdopen(err, false);
        }
        pd->pid = int(cpid);
        pd->errno_fd = std::exchange(fd_errno[1], -1);
    }
}

OSTD_EXPORT void subprocess::reset() {
    if (!p_current) {
        return;
    }
    data *pd = static_cast<data *>(p_current);
    if (pd->errno_fd >= 0) {
        ::close(pd->errno_fd);
    }
    p_current = nullptr;
}

OSTD_EXPORT int subprocess::close() {
    if (!p_current) {
        throw process_error{ECHILD, std::generic_category()};
    }
    data *pd = static_cast<data *>(p_current);
    int retc = 0;
    if (pid_t wp; (wp = waitpid(pd->pid, &retc, 0)) < 0) {
        reset();
        throw process_error{errno, std::generic_category()};
    }
    if (retc) {
        int eno;
        auto r = read(pd->errno_fd, &eno, sizeof(int));
        reset();
        if (r < 0) {
            throw process_error{errno, std::generic_category()};
        } else if (r == sizeof(int)) {
            throw process_error{eno, std::generic_category()};
        }
    }
    reset();
    return retc;
}

#else /* OSTD_PLATFORM_WIN32 */

struct data {
    HANDLE process = nullptr, thread = nullptr;
};

OSTD_EXPORT void subprocess::open_impl(
    std::string const &, std::vector<std::string> const &, bool
) {
    return;
}

OSTD_EXPORT int subprocess::close() {
    throw process_error{ECHILD, std::generic_category()};
}

OSTD_EXPORT void subprocess::reset() {
}

#endif

OSTD_EXPORT void subprocess::move_data(subprocess &i) {
    data *od = static_cast<data *>(i.p_current);
    if (!od) {
        return;
    }
    p_current = ::new (reinterpret_cast<void *>(&p_data)) data{*od};
    i.p_current = nullptr;
}

OSTD_EXPORT void subprocess::swap_data(subprocess &i) {
    if (!p_current) {
        move_data(i);
    } else if (!i.p_current) {
        i.move_data(*this);
    } else {
        std::swap(
            *static_cast<data *>(p_current), *static_cast<data *>(i.p_current)
        );
    }
}

OSTD_EXPORT subprocess::~subprocess() {
    try {
        close();
    } catch (process_error const &) {}
    reset();
}

} /* namespace ostd */
