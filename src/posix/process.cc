/* Process handling implementation bits.
 * For POSIX systems only, other implementations are stored elsewhere.
 *
 * This file is part of libostd. See COPYING.md for futher information.
 */

#include "ostd/platform.hh"

#ifndef OSTD_PLATFORM_POSIX
#  error "Incorrect platform"
#endif

#include <cstddef>
#include <cstdlib>
#include <cerrno>
#include <system_error>
#include <string>
#include <memory>
#include <new>

#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <wordexp.h>

#include "ostd/process.hh"
#include "ostd/format.hh"

namespace ostd {
namespace detail {

OSTD_EXPORT void split_args_impl(
    string_range const &str, void (*func)(string_range, void *), void *data
) {
    if (!str.size()) {
        return;
    }
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
}

} /* namespace detail */

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

    void open(subprocess_stream use) {
        if (use != subprocess_stream::PIPE) {
            return;
        }
        if (::pipe(fd) < 0) {
            throw subprocess_error{"could not open pipe"};
        }
    }

    int &operator[](std::size_t idx) {
        return fd[idx];
    }

    void fdopen(file_stream &s, bool write) {
        FILE *p = ::fdopen(fd[std::size_t(write)], write ? "w" : "r");
        if (!p) {
            throw subprocess_error{"could not open redirected stream"};
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
    bool use_path, string_range cmd,
    bool (*func)(string_range &, void *), void *datap,
    bool (*efunc)(string_range &, void *), void *edatap
) {
    if (use_in == subprocess_stream::STDOUT) {
        throw subprocess_error{"could not redirect stdin to stdout"};
    }

    struct argpv {
        char *cmd = nullptr;
        std::vector<char *> vec;
        ~argpv() {
            if (vec.empty()) {
                return;
            }
            if (cmd && (cmd != vec[0])) {
                delete[] cmd;
            }
            for (char *p: vec) {
                if (p) {
                    delete[] p;
                }
            }
        }
    } argp;

    for (string_range r; func(r, datap);) {
        auto sz = r.size();
        char *str = new char[sz + 1];
        std::char_traits<char>::copy(str, r.data(), sz)[sz] = '\0';
        argp.vec.push_back(str);
    }

    if (argp.vec.empty()) {
        throw subprocess_error{"no arguments given"};
    }

    if (cmd.empty()) {
        argp.cmd = argp.vec[0];
        if (!*argp.cmd) {
            throw subprocess_error{"no command given"};
        }
    } else {
        auto sz = cmd.size();
        char *str = new char[sz + 1];
        std::char_traits<char>::copy(str, cmd.data(), sz)[sz] = '\0';
        argp.cmd = str;
    }

    /* terminate args */
    argp.vec.push_back(nullptr);
    char **argpp = argp.vec.data();

    /* environment */
    char **envp = nullptr;
    if (edatap) {
        auto vsz = argp.vec.size();
        for (string_range r; efunc(r, edatap);) {
            auto sz = r.size();
            char *str = new char[sz + 1];
            std::char_traits<char>::copy(str, r.data(), sz)[sz] = '\0';
            argp.vec.push_back(str);
        }
        /* terminate environment */
        argp.vec.push_back(nullptr);
        envp = &argp.vec[vsz];
    }

    /* fd_errno used to detect if exec failed */
    pipe fd_errno, fd_stdin, fd_stdout, fd_stderr;

    fd_errno.open(subprocess_stream::PIPE);
    fd_stdin.open(use_in);
    fd_stdout.open(use_out);
    fd_stderr.open(use_err);

    auto cpid = fork();
    if (cpid == -1) {
        throw subprocess_error{"fork failed"};
    } else if (!cpid) {
        /* child process */
        fd_errno.close(false);
        /* fcntl fails, write the errno to be read from parent */
        if (fcntl(fd_errno[1], F_SETFD, FD_CLOEXEC) < 0) {
            fd_errno.write_errno();
            std::exit(1);
        }
        /* prepare standard streams */
        if (use_in == subprocess_stream::PIPE) {
            fd_stdin.close(true);
            if (!fd_stdin.dup2(STDIN_FILENO, fd_errno, false)) {
                std::exit(1);
            }
        }
        if (use_out == subprocess_stream::PIPE) {
            fd_stdout.close(false);
            if (!fd_stdout.dup2(STDOUT_FILENO, fd_errno, true)) {
                std::exit(1);
            }
        }
        if (use_err == subprocess_stream::PIPE) {
            fd_stderr.close(false);
            if (!fd_stderr.dup2(STDERR_FILENO, fd_errno, true)) {
                std::exit(1);
            }
        } else if (use_err == subprocess_stream::STDOUT) {
            if (dup2(STDOUT_FILENO, STDERR_FILENO) < 0) {
                fd_errno.write_errno();
                std::exit(1);
            }
        }
        if (use_path) {
            if (envp) {
                execvpe(argp.cmd, argpp, envp);
            } else {
                execvp(argp.cmd, argpp);
            }
        } else {
            if (envp) {
                execve(argp.cmd, argpp, envp);
            } else {
                execv(argp.cmd, argpp);
            }
        }
        /* exec has returned, so error has occured */
        fd_errno.write_errno();
        std::exit(1);
    } else {
        /* parent process */
        fd_errno.close(true);
        if (use_in == subprocess_stream::PIPE) {
            fd_stdin.close(false);
            fd_stdin.fdopen(in, true);
        }
        if (use_out == subprocess_stream::PIPE) {
            fd_stdout.close(true);
            fd_stdout.fdopen(out, false);
        }
        if (use_err == subprocess_stream::PIPE) {
            fd_stderr.close(true);
            fd_stderr.fdopen(err, false);
        }
        p_current = ::new (reinterpret_cast<void *>(&p_data)) data{
            int(cpid), std::exchange(fd_errno[1], -1)
        };
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
        throw subprocess_error{"no child process"};
    }
    data *pd = static_cast<data *>(p_current);
    int retc = 0;
    if (pid_t wp; (wp = waitpid(pd->pid, &retc, 0)) < 0) {
        reset();
        throw subprocess_error{"child process wait failed"};
    }
    if (retc) {
        int eno;
        auto r = read(pd->errno_fd, &eno, sizeof(int));
        reset();
        if (r < 0) {
            return retc;
        } else if (r != sizeof(int)) {
            throw subprocess_error{"could not read from pipe"};
        } else {
            auto ec = std::system_category().default_error_condition(eno);
            auto app = appender<std::string>();
            format(app, "could not execute subprocess (%s)", ec.message());
            throw subprocess_error{std::move(app.get())};
        }
    }
    reset();
    return retc;
}

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

} /* namespace ostd */
