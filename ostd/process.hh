/** @addtogroup Utilities
 * @{
 */

/** @file process.hh
 *
 * @brief Portable extensions to process handling.
 *
 * Provides POSIX and Windows abstractions for process creation and more.
 *
 * @copyright See COPYING.md in the project tree for further information.
 */

#ifndef OSTD_PROCESS_HH
#define OSTD_PROCESS_HH

#include <stdexcept>
#include <system_error>
#include <type_traits>
#include <utility>
#include <string>
#include <vector>

#include "ostd/platform.hh"
#include "ostd/string.hh"
#include "ostd/range.hh"
#include "ostd/io.hh"

namespace ostd {

/** @addtogroup Utilities
 * @{
 */

struct word_error: std::runtime_error {
    using std::runtime_error::runtime_error;
};

namespace detail {
    OSTD_EXPORT void split_args_impl(
        string_range const &str, void (*func)(string_range, void *), void *data
    );
}

/** @brief Splits command line argument string into individual arguments.
 *
 * The split is done in a platform specific manner, using wordexp on POSIX
 * systems and CommandLineToArgvW on Windows. On Windows, the input string
 * is assumed to be UTF-8 and the output strings are always UTF-8, on POSIX
 * the wordexp implementation is followed (so it's locale specific).
 *
 * The `out` argument is an output range that takes a single argument at
 * a time. The value type is any that can be explicitly constructed from
 * an ostd::string_range. However, the string range that is used internally
 * during the conversions is just temporary and freed at the end of this
 * function, so it's important that the string type holds its own memory;
 * an std::string will usually suffice.
 *
 * The ostd::word_error exception is used to handle failures of this
 * function itself. It may also throw other exceptions though, particularly
 * those thrown by `out` when putting the strings into it and also
 * std::bad_alloc on allocation failures.
 *
 * @returns The forwarded `out`.
 *
 * @throws ostd::word_error on failure or anything thrown by `out`.
 * @throws std::bad_alloc on alloation failures.
 */
template<typename OutputRange>
OutputRange &&split_args(OutputRange &&out, string_range str) {
    detail::split_args_impl(str, [](string_range val, void *outp) {
        static_cast<std::decay_t<OutputRange> *>(outp)->put(
            range_value_t<std::decay_t<OutputRange>>{val}
        );
    }, &out);
    return std::forward<OutputRange>(out);
}

struct process_error: std::system_error {
    using std::system_error::system_error;
};

enum class process_pipe {
    DEFAULT = 0,
    PIPE,
    STDOUT
};

struct OSTD_EXPORT process_info {
    process_pipe use_in  = process_pipe::DEFAULT;
    process_pipe use_out = process_pipe::DEFAULT;
    process_pipe use_err = process_pipe::DEFAULT;
    file_stream in  = file_stream{};
    file_stream out = file_stream{};
    file_stream err = file_stream{};

    process_info() {}
    process_info(
        process_pipe in_use, process_pipe out_use, process_pipe err_use
    ):
        use_in(in_use), use_out(out_use), use_err(err_use)
    {}

    process_info(process_info &&i):
        use_in(i.use_in), use_out(i.use_out), use_err(i.use_err),
        in(std::move(i.in)), out(std::move(i.out)), err(std::move(i.err)),
        pid(i.pid), errno_fd(i.errno_fd)
    {
        i.pid = -1;
        i.errno_fd = -1;
    }

    process_info &operator=(process_info &&i) {
        swap(i);
        return *this;
    }

    void swap(process_info &i) {
        using std::swap;
        swap(use_in, i.use_in);
        swap(use_out, i.use_out);
        swap(use_err, i.use_err);
        swap(in, i.in);
        swap(out, i.out);
        swap(err, i.err);
        swap(pid, i.pid);
        swap(errno_fd, i.errno_fd);
    }

    ~process_info();

    int close();

    template<typename InputRange>
    void open_path(string_range path, InputRange &&args) {
        open_full(path, std::forward<InputRange>(args), false);
    }

    template<typename InputRange>
    void open_path(InputRange &&args) {
        open_path(nullptr, std::forward<InputRange>(args));
    }

    template<typename InputRange>
    void open_command(string_range cmd, InputRange &&args) {
        open_full(cmd, std::forward<InputRange>(args), true);
    }

    template<typename InputRange>
    void open_command(InputRange &&args) {
        open_command(nullptr, std::forward<InputRange>(args));
    }

private:
    template<typename InputRange>
    void open_full(string_range cmd, InputRange args, bool use_path) {
        static_assert(
            std::is_constructible_v<std::string, range_reference_t<InputRange>>,
            "The arguments must be strings"
        );
        std::vector<std::string> argv;
        if (cmd.empty()) {
            if (args.empty()) {
                throw process_error{EINVAL, std::generic_category()};
            }
            cmd = args[0];
        }
        for (; !args.empty(); args.pop_front()) {
            argv.emplace_back(args.front());
        }
        open_impl(std::string{cmd}, argv, use_path);
    }

    void open_impl(
        std::string const &cmd, std::vector<std::string> const &args,
        bool use_path
    );
    int pid = -1, errno_fd = -1;
};

/** @} */

} /* namespace ostd */

#endif

/** @} */
