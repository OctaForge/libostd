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

static void stdstream_close(FILE *f) {
    std::fclose(f);
}

OSTD_EXPORT void process_info::open_impl(
    std::string const &cmd, std::vector<std::string> const &args,
    bool use_path
) {
    if (use_in == process_stream::STDOUT) {
        throw process_error{EINVAL, std::generic_category()};
    }

#ifndef OSTD_PLATFORM_WIN32
    auto argp = std::make_unique<char *[]>(args.size() + 1);
    for (std::size_t i = 0; i < args.size(); ++i) {
        argp[i] = const_cast<char *>(args[i].data());
    }
    argp[args.size()] = nullptr;

    int fd_errno[2]; /* used to detect if exec failed */
    int fd_stdin[2];
    int fd_stdout[2];
    int fd_stderr[2];

    if (
        (pipe(fd_errno) < 0) ||
        ((use_in == process_stream::PIPE) && (pipe(fd_stdin) < 0)) ||
        ((use_out == process_stream::PIPE) && (pipe(fd_stdout) < 0)) ||
        ((use_err == process_stream::PIPE) && (pipe(fd_stderr) < 0))
    ) {
        throw process_error{errno, std::generic_category()};
    }

    auto cpid = fork();
    if (cpid == -1) {
        throw process_error{errno, std::generic_category()};
    } else if (!cpid) {
        /* child process */
        ::close(fd_errno[0]);
        /* fcntl fails, write the errno to be read from parent */
        if (fcntl(fd_errno[1], F_SETFD, FD_CLOEXEC) < 0) {
            write(fd_errno[1], int(errno), sizeof(int));
            std::exit(1);
        }
        /* prepare standard streams */
        if (use_in == process_stream::PIPE) {
            /* close writing end */
            ::close(fd_stdin[1]);
            if (dup2(fd_stdin[0], STDIN_FILENO) < 0) {
                write(fd_errno[1], int(errno), sizeof(int));
                std::exit(1);
            }
        }
        if (use_out == process_stream::PIPE) {
            /* close reading end */
            ::close(fd_stdout[0]);
            if (dup2(fd_stdout[1], STDOUT_FILENO) < 0) {
                write(fd_errno[1], int(errno), sizeof(int));
                std::exit(1);
            }
        }
        if (use_err == process_stream::PIPE) {
            /* close reading end */
            ::close(fd_stderr[0]);
            if (dup2(fd_stderr[1], STDERR_FILENO) < 0) {
                write(fd_errno[1], int(errno), sizeof(int));
                std::exit(1);
            }
        } else if (use_err == process_stream::STDOUT) {
            if (dup2(STDOUT_FILENO, STDERR_FILENO) < 0) {
                write(fd_errno[1], int(errno), sizeof(int));
                std::exit(1);
            }
        }
        int ret = 0;
        if (use_path) {
            ret = execvp(cmd.data(), argp.get());
        } else {
            ret = execv(cmd.data(), argp.get());
        }
        std::exit(1);
    } else {
        /* parent process */
        ::close(fd_errno[1]);
        if (use_in == process_stream::PIPE) {
            /* close reading end */
            ::close(fd_stdin[0]);
            in.open(fdopen(fd_stdin[1], "w"), stdstream_close);
        }
        if (use_out == process_stream::PIPE) {
            /* close writing end */
            ::close(fd_stdout[1]);
            out.open(fdopen(fd_stdout[0], "r"), stdstream_close);
        }
        if (use_err == process_stream::PIPE) {
            /* close writing end */
            ::close(fd_stderr[1]);
            err.open(fdopen(fd_stderr[0], "r"), stdstream_close);
        }
        pid = int(cpid);
        errno_fd = fd_errno[1];
    }
#else
    /* stub for now */
    return;
#endif
}

OSTD_EXPORT int process_info::close() {
    if (pid < 0) {
        throw process_error{ECHILD, std::generic_category()};
    }
#ifndef OSTD_PLATFORM_WIN32
    int retc = 0;
    if (pid_t wp; (wp = waitpid(pid, &retc, 0)) < 0) {
        throw process_error{errno, std::generic_category()};
    }
    if (retc) {
        int eno;
        auto r = read(errno_fd, &eno, sizeof(int));
        ::close(errno_fd);
        errno_fd = -1;
        if (r < 0) {
            throw process_error{errno, std::generic_category()};
        } else if (r == sizeof(int)) {
            throw process_error{eno, std::generic_category()};
        }
    }
    return retc;
#else
    throw process_error{ECHILD, std::generic_category()};
#endif
}

OSTD_EXPORT process_info::~process_info() {
    try {
        close();
    } catch (process_error const &) {}
#ifndef OSTD_PLATFORM_WIN32
    if (errno_fd > 0) {
        ::close(errno_fd);
    }
#else
    return;
#endif
}

} /* namespace ostd */
