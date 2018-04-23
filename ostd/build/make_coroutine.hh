/** @addtogroup Build
 * @{
 */

/** @file make_coroutine.hh
 *
 * @brief Coroutine integration for ostd::build::make.
 *
 * This file provides an implementation of coroutine-based make tasks,
 * allowing for seamless synchronous-looking build rules.
 *
 * @copyright See COPYING.md in the project tree for further information.
 */

#ifndef OSTD_BUILD_MAKE_COROUTINE_HH
#define OSTD_BUILD_MAKE_COROUTINE_HH

#include <vector>
#include <future>
#include <chrono>

#include <ostd/string.hh>
#include <ostd/coroutine.hh>
#include <ostd/build/make.hh>

namespace ostd {
namespace build {

/** @addtogroup Build
 * @{
 */

namespace detail {
    struct make_task_coro: make_task {
        make_task_coro() = delete;

        make_task_coro(
            string_range target, std::vector<string_range> deps, make_rule &rl
        ): p_coro(
            [target, deps = std::move(deps), &rl](auto) mutable {
                rl.call(target, iterator_range<string_range *>(
                    deps.data(), deps.data() + deps.size()
                ));
            }
        ) {}

        bool done() const {
            return !p_coro;
        }

        void resume() {
            p_coro.resume();
        }

        std::shared_future<void> add_task(std::future<void> f) {
            for (;;) {
                auto fs = f.wait_for(std::chrono::seconds(0));
                if (fs != std::future_status::ready) {
                    /* keep yielding until ready */
                    auto &cc = static_cast<coroutine<void()> &>(
                        *coroutine_context::current()
                    );
                    (coroutine<void()>::yield_type(cc))();
                } else {
                    break;
                }
            }
            /* maybe propagate exception */
            f.get();
            /* future is done, nothing to actually share */
            return std::shared_future<void>{};
        }

    private:
        coroutine<void()> p_coro;
    };
}

inline make_task *make_task_coroutine(
    string_range target, std::vector<string_range> deps, make_rule &rl
) {
    return new detail::make_task_coro{target, std::move(deps), rl};
}

/** @} */

} /* namespace build */
} /* namespace ostd */

#endif

/** @} */
