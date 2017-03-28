/** @addtogroup Concurrency
 * @{
 */

/** @file coroutine.hh
 *
 * @brief Lightweight cooperative threads.
 *
 * This file implements coroutines (sometimes also called fibers or green
 * threads), which are essentially very lightweight cooperative threads.
 * They're designed with very low context switch overhead in mind, so that
 * they can be spawned in large numbers. Coupled with a scheduler, it's
 * possible to create scalable concurrent/parallel applications. They can
 * also be used as generators and generally anywhere where you'd like to
 * suspend a function and resume it later (all kinds of async apps).
 *
 * @copyright See COPYING.md in the project tree for further information.
 */

#ifndef OSTD_COROUTINE_HH
#define OSTD_COROUTINE_HH

#include <exception>
#include <stdexcept>
#include <typeinfo>
#include <utility>
#include <tuple>
#include <type_traits>
#include <optional>

#include "ostd/platform.hh"
#include "ostd/types.hh"
#include "ostd/range.hh"

#include "ostd/context_stack.hh"

namespace ostd {

/** @addtogroup Concurrency
 * @{
 */

/** @brief Thrown on coroutine related errors.
 *
 * These can include a dead coroutine/generator call/access.
 */
struct coroutine_error: std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct coroutine_context;

namespace detail {
    /* from boost.fcontext */
    using fcontext_t = void *;

    struct transfer_t {
        fcontext_t ctx;
        void *data;
    };

    extern "C" OSTD_EXPORT
    transfer_t OSTD_CDECL ostd_jump_fcontext(
        fcontext_t const to, void *vp
    );

    extern "C" OSTD_EXPORT
    fcontext_t OSTD_CDECL ostd_make_fcontext(
        void *sp, size_t size, void (*fn)(transfer_t)
    );

    extern "C" OSTD_EXPORT
    transfer_t OSTD_CDECL ostd_ontop_fcontext(
        fcontext_t const to, void *vp, transfer_t (*fn)(transfer_t)
    );

    OSTD_EXPORT extern thread_local coroutine_context *coro_current;
} /* namespace detail */

/** @brief An encapsulated context any coroutine-type inherits from.
 *
 * Internally, this provides some basic infrastructure for creating and
 * managing the coroutine as well as its entry point. All coroutine types
 * inherit from this.
 */
struct coroutine_context {
    /** @brief Gets the currently executing coroutine context.
     *
     * Sometimes custom coroutine types might want to bypass being able
     * to be accessed through this, so they can choose to avoid calling
     * call(). The coroutine-based concurrent schedulers particularly do
     * this. This means that if you call this while in a task, it will
     * return either `nullptr` or the non-task coroutine we're currently in.
     *
     * @returns The current context.
     */
    static coroutine_context *current() {
        return detail::coro_current;
    }

protected:
    /** Does nothing, just default-initializes the members. */
    coroutine_context() {}

    /** @brief Unwinds the stack and frees it.
     *
     * If the coroutine is dead or just initialized and was never called,
     * no unwind is performed (in the former case, it's already done, in
     * the latter case, it was never needed in the first place). Then the
     * stack is freed (if present) using the same stack allocator it was
     * allocated with.
     */
    virtual ~coroutine_context() {
        unwind();
        free_stack();
    }

    /** Coroutine contexts are not copy constructible. */
    coroutine_context(coroutine_context const &) = delete;

    /** @brief Moves the given context into this one.
     *
     * The other context is deinitialized and set as dead.
     *
     * @see operator=(coroutine_context &&)
     */
    coroutine_context(coroutine_context &&c):
        p_stack(std::move(c.p_stack)), p_coro(c.p_coro), p_orig(c.p_orig),
        p_except(std::move(c.p_except)), p_state(c.p_state), p_free(c.p_free)
    {
        c.p_coro = c.p_orig = nullptr;
        c.p_stack = { nullptr, 0 };
        c.p_free = nullptr;
        c.set_dead();
    }

    /** Coroutine contexts are not copy assignable. */
    coroutine_context &operator=(coroutine_context const &) = delete;

    /** @brief Moves the given context into this one.
     *
     * Effectively calls swap(coroutine_context &). The current context
     * is then destroyed by the other context's destructor.
     *
     * @returns Returns `*this*`.
     */
    coroutine_context &operator=(coroutine_context &&c) {
        swap(c);
        return *this;
    }

    /** @brief Calls the coroutine context.
     *
     * Sets the state flag as executing, saves the current context and
     * sets it to this coroutine (so that current() returns this context)
     * and jumps into the coroutine function. Once the coroutine yields or
     * returns, sets the current context back to what was saved previously.
     *
     * Additionally, propagates any uncaught exception that was thrown inside
     * of the coroutine function by calling rethrow().
     *
     * @throws Whatever the coroutine function throws.
     */
    void call() {
        set_exec();
        /* switch to new coroutine */
        coroutine_context *curr = std::exchange(detail::coro_current, this);
        coro_jump();
        /* switch back to previous */
        detail::coro_current = curr;
        rethrow();
    }

    /** @brief Jumps into the coroutine context.
     *
     * Only valid if the coroutine is currently suspended or not started.
     *
     * @see yield_jump(), yield_done()
     */
    void coro_jump() {
        p_coro = detail::ostd_jump_fcontext(p_coro, this).ctx;
    }

    /** @brief Jumps out of the coroutine context.
     *
     * Only valid if the coroutine is currently running. Sets the state
     * to suspended before jumping.
     *
     * @see coro_jump(), yield_done()
     */
    void yield_jump() {
        p_state = state::HOLD;
        p_orig = detail::ostd_jump_fcontext(p_orig, nullptr).ctx;
    }

    /** @brief Jumps out of the coroutine context.
     *
     * Only valid if the coroutine is currently running. Sets the state
     * to dead before jumping.
     *
     * @see coro_jump(), yield_jump()
     */
    void yield_done() {
        set_dead();
        p_orig = detail::ostd_jump_fcontext(p_orig, nullptr).ctx;
    }

    /** Checks if the coroutine is suspended. */
    bool is_hold() const {
        return (p_state == state::HOLD);
    }

    /** Checks if the coroutine is dead. */
    bool is_dead() const {
        return (p_state == state::TERM);
    }

    /** Marks the coroutine as dead. Only valid without an active context. */
    void set_dead() {
        p_state = state::TERM;
    }

    /** Marks the coroutine as executing. Only valid before coro_jump(). */
    void set_exec() {
        p_state = state::EXEC;
    }

    /** @brief Rethrows an exception saved by a previous coroutine call.
     *
     * Call this after coro_jump() when implementing your own call().
     * When not implementing your own call(), do not do this anywhere,
     * the exception is already rethrown inside call().
     */
    void rethrow() {
        if (p_except) {
            std::rethrow_exception(std::move(p_except));
        }
    }

    /** Swaps the current context with another. */
    void swap(coroutine_context &other) noexcept {
        using std::swap;
        swap(p_stack, other.p_stack);
        swap(p_coro, other.p_coro);
        swap(p_orig, other.p_orig);
        swap(p_except, other.p_except);
        swap(p_state, other.p_state);
        swap(p_free, other.p_free);
    }

    /** @brief Allocates a stack and creates a context.
     *
     * Only call this once at the point where no context was created yet.
     * That is typically in a constructor that results in a valid coroutine.
     *
     * The context uses an internal entry point. The entry point calls the
     * coroutine's function by calling the `resume_call()` method with no
     * parameters and results on it. Any exception thrown from it is caught
     * and saved as an `std::exception_ptr`. This is then rethrown by a call
     * to rethrow().
     *
     * It's therefore necessary for @p C to give the context access to its
     * `resume_call()` method, typically by making coroutine_context a friend.
     *
     * @param sa The stack allocator used to allocate the stack.
     * @tparam C The coroutine type that inherits from the context class.
     */
    template<typename C, typename SA>
    void make_context(SA &sa) {
        p_stack = sa.allocate();

        void *sp = get_stack_ptr<SA>();
        size_t asize = p_stack.size -
            (static_cast<byte *>(p_stack.ptr) - static_cast<byte *>(sp));

        p_coro = detail::ostd_make_fcontext(sp, asize, &context_call<C, SA>);
        new (sp) SA(std::move(sa));
        p_free = &free_stack_call<SA>;
    }

private:
    /* we also store the stack allocator at the end of the stack */
    template<typename SA>
    void *get_stack_ptr() {
        /* 16 byte stack pointer alignment */
        constexpr size_t salign = 16;
        /* makes enough space so that we can store the allocator at
         * stack pointer location (we'll need it for dealloc later)
         */
        constexpr size_t sasize = sizeof(SA);

        void *sp = static_cast<byte *>(p_stack.ptr) - sasize - salign;
        size_t space = sasize + salign;
        sp = std::align(salign, sasize, sp, space);

        return sp;
    }

    struct forced_unwind {
        detail::fcontext_t ctx;
        forced_unwind(detail::fcontext_t c): ctx(c) {}
    };

    enum class state {
        HOLD = 0, EXEC, TERM
    };

    void unwind() {
        if (is_dead()) {
            /* this coroutine was either initialized with a null function or
             * it's already terminated and thus its stack has already unwound
             */
            return;
        }
        if (!p_orig) {
            /* this coroutine never got to live :( */
            return;
        }
        detail::ostd_ontop_fcontext(
            std::exchange(p_coro, nullptr), nullptr,
            [](detail::transfer_t t) -> detail::transfer_t {
                throw forced_unwind{t.ctx};
            }
        );
    }

    template<typename SA>
    static void free_stack_call(void *data) {
        auto &self = *(static_cast<coroutine_context *>(data));
        auto &sa = *(static_cast<SA *>(self.get_stack_ptr<SA>()));
        SA dsa{std::move(sa)};
        sa.~SA();
        dsa.deallocate(self.p_stack);
    }

    void free_stack() {
        if (p_free) {
            p_free(this);
        }
    }

    template<typename C, typename SA>
    static void context_call(detail::transfer_t t) {
        auto &self = *(static_cast<C *>(t.data));
        self.p_orig = t.ctx;
        if (self.is_hold()) {
            /* we never got to execute properly, we're HOLD because we
             * jumped here without setting the state to EXEC before that
             */
            goto release;
        }
        try {
            self.resume_call();
        } catch (coroutine_context::forced_unwind v) {
            /* forced_unwind is unique */
            self.p_orig = v.ctx;
        } catch (...) {
            /* some other exception, will be rethrown later */
            self.p_except = std::current_exception();
        }
        /* switch back, release stack */
release:
        self.yield_done();
    }

    stack_context p_stack;
    detail::fcontext_t p_coro = nullptr;
    detail::fcontext_t p_orig = nullptr;
    std::exception_ptr p_except;
    state p_state = state::HOLD;
    void (*p_free)(void *) = nullptr;
};

/** @brief Unspecialized declaration of the coroutine.
 *
 * This one has no definition. All instances of coroutine must use `R(A...)`
 * as template arguments, which is specialized later.
 */
template<typename T>
struct coroutine;

namespace detail {
    template<typename T>
    using coro_arg = std::add_pointer_t<std::remove_reference_t<T>>;

    /* dealing with args */
    template<typename ...A>
    struct coro_types {
        using yield_type = std::tuple<A...>;

        static yield_type get(std::tuple<coro_arg<A>...> &args) {
            return std::apply([](auto ...args) {
                return std::make_tuple(std::forward<A>(*args)...);
            }, args);
        }
    };

    template<typename A>
    struct coro_types<A> {
        using yield_type = A;

        static yield_type get(std::tuple<coro_arg<A>> &args) {
            return std::forward<A>(*std::get<0>(args));
        }
    };

    template<typename A, typename B>
    struct coro_types<A, B> {
        using yield_type = std::pair<A, B>;

        static yield_type get(std::tuple<coro_arg<A>, coro_arg<B>> &args) {
            return std::make_pair(
                std::forward<A>(*std::get<0>(args)),
                std::forward<B>(*std::get<1>(args))
            );
        }
    };

    template<>
    struct coro_types<> {
        using yield_type = void;
    };

    template<typename ...A>
    using coro_args = typename coro_types<A...>::yield_type;

    /* storing and handling results */
    template<typename R>
    struct coro_rtype {
        using type = std::aligned_storage_t<sizeof(R), alignof(R)>;

        static void store(type &stor, R &&v) {
            new (&stor) R(std::move(v));
        }

        static void store(type &stor, R const &v) {
            new (&stor) R(v);
        }

        static R get(type &stor) {
            R &tstor = *reinterpret_cast<R *>(&stor);
            R ret{std::forward<R>(tstor)};
            /* this way we can make sure result is always uninitialized
             * except when resuming, so no need to keep a bool flag etc.
             */
            tstor.~R();
            return ret;
        }
    };

    template<typename R>
    struct coro_rtype<R &> {
        using type = R *;

        static void store(type &stor, R &v) {
            stor = &v;
        }

        static R &get(type stor) {
            return *stor;
        }
    };

    template<typename R>
    struct coro_rtype<R &&> {
        using type = std::aligned_storage_t<sizeof(R), alignof(R)>;

        static void store(type &stor, R &&v) {
            new (&stor) R(std::move(v));
        }

        static R &&get(type &stor) {
            R &tstor = *reinterpret_cast<R *>(&stor);
            R ret{std::forward<R>(tstor)};
            tstor.~R();
            return std::move(ret);
        }
    };

    template<typename R>
    using coro_result = typename coro_rtype<R>::type;

    /* default case, yield returns args and takes a value */
    template<typename Y, typename R, typename ...A>
    struct coro_stor {
        coro_stor(): p_func(nullptr) {}
        template<typename F>
        coro_stor(F &&func): p_func(std::forward<F>(func)) {}

        coro_args<A...> get_args() {
            return coro_types<A...>::get(p_args);
        }

        void set_args(coro_arg<A> ...args) {
            p_args = std::make_tuple(args...);
        }

        R get_result() {
            return std::forward<R>(coro_rtype<R>::get(p_result));
        }

        template<typename C>
        void call_helper(C &coro) {
            std::apply([this, &coro](auto ...args) {
                coro_rtype<R>::store(p_result, std::forward<R>(p_func(
                    Y{coro}, std::forward<A>(*args)...
                )));
            }, p_args);
        }

        void swap(coro_stor &other) {
            using std::swap;
            swap(p_func, other.p_func);
            swap(p_args, other.p_args);
            /* no need to swap result as result is always only alive
             * for the time of a single resume, in which no swap happens
             */
        }

        std::function<R(Y, A...)> p_func;
        std::tuple<coro_arg<A>...> p_args;
        coro_result<R> p_result;
    };

    /* yield takes a value but doesn't return any args */
    template<typename Y, typename R>
    struct coro_stor<Y, R> {
        coro_stor(): p_func(nullptr) {}
        template<typename F>
        coro_stor(F &&func): p_func(std::forward<F>(func)) {}

        void get_args() {}
        void set_args() {}

        R get_result() {
            return std::forward<R>(coro_rtype<R>::get(p_result));
        }

        template<typename C>
        void call_helper(C &coro) {
            coro_rtype<R>::store(
                p_result, std::forward<R>(p_func(Y{coro}))
            );
        }

        void swap(coro_stor &other) {
            std::swap(p_func, other.p_func);
        }

        std::function<R(Y)> p_func;
        coro_result<R> p_result;
    };

    /* yield doesn't take a value and returns args */
    template<typename Y, typename ...A>
    struct coro_stor<Y, void, A...> {
        coro_stor(): p_func(nullptr) {}
        template<typename F>
        coro_stor(F &&func): p_func(std::forward<F>(func)) {}

        coro_args<A...> get_args() {
            return coro_types<A...>::get(p_args);
        }

        void set_args(coro_arg<A> ...args) {
            p_args = std::make_tuple(args...);
        }

        void get_result() {}

        template<typename C>
        void call_helper(C &coro) {
            std::apply([this, &coro](auto ...args) {
                p_func(Y{coro}, std::forward<A>(*args)...);
            }, p_args);
        }

        void swap(coro_stor &other) {
            using std::swap;
            swap(p_func, other.p_func);
            swap(p_args, other.p_args);
        }

        std::function<void(Y, A...)> p_func;
        std::tuple<coro_arg<A>...> p_args;
    };

    /* yield doesn't take a value or return any args */
    template<typename Y>
    struct coro_stor<Y, void> {
        coro_stor(): p_func(nullptr) {}
        template<typename F>
        coro_stor(F &&func): p_func(std::forward<F>(func)) {}

        void get_args() {}
        void set_args() {}

        void get_result() {}

        template<typename C>
        void call_helper(C &coro) {
            p_func(Y{coro});
        }

        void swap(coro_stor &other) {
            std::swap(p_func, other.p_func);
        }

        std::function<void(Y)> p_func;
    };
} /* namespace detail */

/** @brief The coroutine type specialization.
 *
 * This is a regular coroutine. Depending on the template param, it can
 * have a return type as well as arguments. The typical usage is as follows:
 *
 * ~~~{.cc}
 * coroutine<int(float, bool)> x = [](auto yield, float a, bool b) {
 *     writefln("call 1: %s %s", a, b);
 *     std::tie(a, b) = yield(5); // "returns" 5
 *     writefln("call 2: %s %s", a, b);
 *     return 10;
 * };
 *
 * int r = x(3.14f, true); // call 1: 3.14 true
 * writeln(r); // 5
 *
 * r = x(6.28f, false); // call 2: 6.28 false
 * writeln(r); // 10, x is dead here
 * ~~~
 *
 * As you can see, the coroutine function takes an object by value (yield_type)
 * and any declared arguments. Any types can be arguments as well as returns,
 * just like with any function or functional wrapper like `std::function`.
 *
 * You suspend the coroutine by calling its yield type. Depending on the
 * result type, the yielder call takes an argument or not. If `R` is `void`,
 * the yielder takes no argument. If it's a non-reference type, the yielder
 * takes an argument by lvalue reference to `const` (`R const &`) or by rvalue
 * reference (`R &&`).  If it's an lvalue reference type, it takes the lvalue
 * reference type. If it's an rvalue reference type, it takes the rvalue
 * reference.
 *
 * The yielder returns a value or not depending on the argument types. With
 * no argument types, nothing is returned. With a single argument type, the
 * argument itself is returned. With two argument types, an `std::pair` of
 * the arguments is returned. Otherwise, an `std::tuple` of the arguments
 * is returned. The arguments are never copied or moved, you get them exactly
 * as they were passed, exactly as if they were passed as regular args.
 *
 * @tparam R The return template type.
 * @tparam A... The argument template types.
 */
template<typename R, typename ...A>
struct coroutine<R(A...)>: coroutine_context {
private:
    using base_t = coroutine_context;
    /* necessary so that context callback can access privates */
    friend struct coroutine_context;

    template<typename RR, typename ...AA>
    struct yielder {
        yielder(coroutine<RR(AA...)> &coro): p_coro(coro) {}

        detail::coro_args<AA...> operator()(RR &&ret) {
            detail::coro_rtype<RR>::store(
                p_coro.p_stor.p_result, std::move(ret)
            );
            p_coro.yield_jump();
            return p_coro.p_stor.get_args();
        }

        detail::coro_args<AA...> operator()(RR const &ret) {
            detail::coro_rtype<RR>::store(p_coro.p_stor.p_result, ret);
            p_coro.yield_jump();
            return p_coro.p_stor.get_args();
        }

    private:
        coroutine<RR(AA...)> &p_coro;
    };

    template<typename RR, typename ...AA>
    struct yielder<RR &, AA...> {
        yielder(coroutine<RR &(AA...)> &coro): p_coro(coro) {}

        detail::coro_args<AA...> operator()(RR &ret) {
            detail::coro_rtype<RR &>::store(p_coro.p_stor.p_result, ret);
            p_coro.yield_jump();
            return p_coro.p_stor.get_args();
        }

    private:
        coroutine<RR &(AA...)> &p_coro;
    };

    template<typename RR, typename ...AA>
    struct yielder<RR &&, AA...> {
        yielder(coroutine<RR &&(AA...)> &coro): p_coro(coro) {}

        detail::coro_args<AA...> operator()(RR &&ret) {
            detail::coro_rtype<RR &&>::store(
                p_coro.p_stor.p_result, std::move(ret)
            );
            p_coro.yield_jump();
            return p_coro.p_stor.get_args();
        }

    private:
        coroutine<RR &&(AA...)> &p_coro;
    };

    template<typename ...AA>
    struct yielder<void, AA...> {
        yielder(coroutine<void(AA...)> &coro): p_coro(coro) {}

        detail::coro_args<AA...> operator()() {
            p_coro.yield_jump();
            return p_coro.p_stor.get_args();
        }

    private:
        coroutine<void(AA...)> &p_coro;
    };

public:
    /** The yielder type for the coroutine. Not opaque, but internal. */
    using yield_type = yielder<R, A...>;

    /** Coroutines are not default constructible. */
    coroutine() = delete;

    /** @brief Creates a coroutine using the given function.
     *
     * The function is stored in an appropriate `std::function`. If the
     * resulting function is empty, it's the same as initializing the
     * coroutine with a null pointer (i.e. it's dead by default).
     *
     * Otherwise creates a context using the provided stack allocator.
     *
     * @param func The function to use.
     * @param sa The stack allocator, defaults to a default_stack.
     *
     * @throws Whatever an `std::function` constructor could throw.
     */
    template<typename F, typename SA = default_stack>
    coroutine(F func, SA sa = SA{}): base_t(), p_stor(std::move(func)) {
        /* that way there is no context creation/stack allocation */
        if (!p_stor.p_func) {
            this->set_dead();
            return;
        }
        this->make_context<coroutine<R(A...)>>(sa);
    }

    /** @brief Creates a dead coroutine.
     *
     * No context is created. No stack is allocated.
     */
    template<typename SA = default_stack>
    coroutine(std::nullptr_t, SA = SA{}): base_t(), p_stor() {
        this->set_dead();
    }

    /** Coroutines are not copy constructible. */
    coroutine(coroutine const &) = delete;

    /** @brief Moves a coroutine.
     *
     * Moves the given coroutine's context and any other data into the new
     * coroutine. The other coroutine is left in a dead state.
     *
     * @see operator=(coroutine &&)
     */
    coroutine(coroutine &&c) noexcept:
        base_t(std::move(c)), p_stor(std::move(c.p_stor))
    {
        c.p_stor.p_func = nullptr;
    }

    /** Coroutines are not copy assignable. */
    coroutine &operator=(coroutine const &) = delete;

    /** @brief Moves a coroutine.
     *
     * Effectively like a call to swap(coroutine &).
     */
    coroutine &operator=(coroutine &&c) noexcept {
        base_t::operator=(std::move(c));
        p_stor.swap(c.p_stor);
    }

    /** Checks if the coroutine is alive, returning `true` if it is. */
    explicit operator bool() const noexcept {
        return !this->is_dead();
    }

    /** @brief Calls the coroutine.
     *
     * Executes the coroutine with the given arguments and returns whatever
     * the coroutine returns or yields. The arguments are never copied or
     * moved, they're passed into the coroutine exactly as real args would.
     *
     * @returns The value passed to `yield()` (or nothing if `R` is `void`).
     *
     * @throws ostd::coroutine_error if the coroutine is dead.
     *
     * @see operator()(A...)
     */
    R resume(A ...args) {
        if (this->is_dead()) {
            throw coroutine_error{"dead coroutine"};
        }
        this->set_args(&args...);
        base_t::call();
        return this->get_result();
    }

    /** @brief Calls the coroutine.
     *
     * Exactly the same as calling resume(A...), with function syntax.
     *
     * @see resume(A...)
     */
    R operator()(A ...args) {
        /* duplicate the logic so we don't copy/move the args */
        if (this->is_dead()) {
            throw coroutine_error{"dead coroutine"};
        }
        p_stor.set_args(&args...);
        base_t::call();
        return p_stor.get_result();
    }

    /** Swaps two coroutines' states. */
    void swap(coroutine &other) noexcept {
        p_stor.swap(other.p_stor);
        base_t::swap(other);
    }

    /** Returns the RTTI of the function stored in the coroutine. */
    std::type_info const &target_type() const {
        return p_stor.p_func.target_type();
    }

    /** @brief Retrieves the stored function given its known type.
     *
     * @returns A pointer to the function or `nullptr` if the type
     *          doesn't match.
     */
    template<typename F>
    F *target() { return p_stor.p_func.target(); }

    /** @brief Retrieves the stored function given its known type.
     *
     * @returns A pointer to the function or `nullptr` if the type
     *          doesn't match.
     */
    template<typename F>
    F const *target() const { return p_stor.p_func.target(); }

private:
    void resume_call() {
        p_stor.call_helper(*this);
    }

    detail::coro_stor<yield_type, R, A...> p_stor;
};

/** Swaps two coroutines' states. */
template<typename R, typename ...A>
inline void swap(coroutine<R(A...)> &a, coroutine<R(A...)> &b) noexcept {
    a.swap(b);
}

template<typename T> struct generator_range;

namespace detail {
    template<typename T> struct generator_iterator;
}

/** @brief A generator type.
 *
 * Generators are much like coroutines and are also backed by contexts and
 * stacks. However, there are a few important differences:
 *
 * * Generators take no arguments (besides the yielder).
 * * Generators don't return from their function, but pass values to `yield()`.
 * * Generators store a value until resumed, which can be checked many times.
 * * Generators are started automatically and have a value until they die.
 * * Generators are iterable and you can even take ranges to them.
 *
 * This is best described with an example:
 *
 * ~~~{.cc}
 * auto x = [](auto yield) {
 *     for (int i = 1; i <= 5; ++i) {
 *         yield(i * 5);
 *     }
 * };
 *
 * for (int i: generator<int>{x}) {
 *     writeln(i); // 5, 10, 15, 20, 25
 * }
 *
 * generator<int> y = x;
 * writeln(y.value()); // 5
 * writeln(y.value()); // 5
 * y.resume();
 * writeln(y.value()); // 10
 * ~~~
 *
 * As you can see, where coroutines are basically just resumable functions,
 * generators are truly suitable for generating values. 
 *
 * @tparam T The value type to use.
 */
template<typename T>
struct generator: coroutine_context {
private:
    using base_t = coroutine_context;
    friend struct coroutine_context;

    template<typename U>
    struct yielder {
        yielder(generator<U> &g): p_gen(g) {}

        void operator()(U &&ret) {
            p_gen.p_result = &ret;
            p_gen.yield_jump();
        }

        void operator()(U const &ret) {
            if constexpr(std::is_const_v<U>) {
                p_gen.p_result = &ret;
                p_gen.yield_jump();
            } else {
                T val{ret};
                p_gen.p_result = &val;
                p_gen.yield_jump();
            }
        }
    private:
        generator<U> &p_gen;
    };

    template<typename U>
    struct yielder<U &> {
        yielder(generator<U> &g): p_gen(g) {}

        void operator()(U &ret) {
            p_gen.p_result = &ret;
            p_gen.yield_jump();
        }
    private:
        generator<U> &p_gen;
    };

    template<typename U>
    struct yielder<U &&> {
        yielder(generator<U> &g): p_gen(g) {}

        void operator()(U &&ret) {
            p_gen.p_result = &ret;
            p_gen.yield_jump();
        }
    private:
        generator<U> &p_gen;
    };

public:
    /** As generators can be used with ranges, this is defined. */
    using range = generator_range<T>;

    /** The yielder type for the generator. Not opaque, but internal. */
    using yield_type = yielder<T>;

    /** Generators are not default constructible. */
    generator() = delete;

    /** @brief Creates a generator using the given function.
     *
     * The function is stored in an appropriate `std::function`. If the
     * resulting function is empty, it's the same as initializing the
     * generator with a null pointer (i.e. it's dead by default).
     *
     * Otherwise creates a context using the provided stack allocator
     * and then resumes the generator, making it get a value (or die).
     *
     * @param func The function to use.
     * @param sa The stack allocator, defaults to a default_stack.
     *
     * @throws Whatever an `std::function` constructor could throw.
     */
    template<typename F, typename SA = default_stack>
    generator(F func, SA sa = SA{}):
        base_t(), p_func(std::move(func))
    {
        /* that way there is no context creation/stack allocation */
        if (!p_func) {
            this->set_dead();
            return;
        }
        this->make_context<generator<T>>(sa);
        /* generate an initial value */
        resume();
    }

    /** @brief Creates a dead generator.
     *
     * No context is created. No stack is allocated.
     */
    template<typename SA = default_stack>
    generator(std::nullptr_t, SA = SA{0}):
        base_t(), p_func(nullptr)
    {
        this->set_dead();
    }

    /** Generators are not copy constructible. */
    generator(generator const &) = delete;

    /** @brief Moves a generator.
     *
     * Moves the given generator's context and any other data into the new
     * generator. The other generator is left in a dead state.
     *
     * @see operator=(generator &&)
     */
    generator(generator &&c) noexcept:
        base_t(std::move(c)), p_func(std::move(c.p_func)), p_result(c.p_result)
    {
        c.p_func = nullptr;
        c.p_result = nullptr;
    }

    /** Generators are not copy assignable. */
    generator &operator=(generator const &) = delete;

    /** @brief Moves a generator.
     *
     * Effectively like a call to swap(generator &).
     */
    generator &operator=(generator &&c) noexcept {
        base_t::operator=(std::move(c));
        p_func = std::move(c.p_func);
        p_result = c.p_result;
        c.p_func = nullptr;
        c.p_result = nullptr;
    }

    /** Checks if the generator is alive, returning `true` if it is. */
    explicit operator bool() const noexcept {
        return !this->is_dead();
    }

    /** @brief Resumes the generator, going to the next value (or dying).
     *
     * Executes the generator and if it yields, the yielded value will then
     * be accessible via value(). Otherwise, the generator dies and there
     * will not be any value.
     *
     * @throws ostd::coroutine_error if the generator is dead.
     */
    void resume() {
        if (this->is_dead()) {
            throw coroutine_error{"dead generator"};
        }
        coroutine_context::call();
    }

    /** @brief Retrieves a reference to the generator's value.
     *
     * When the generator is in a suspended state, the value it has yielded
     * lies on it stack, which is guaranteed to be alive. Therefore we don't
     * have to copy the value and can return a cheap reference to it.
     *
     * @throws ostd::coroutine_error if the generator is dead.
     */
    T &value() {
        if (!p_result) {
            throw coroutine_error{"no value"};
        }
        return *p_result;
    }

    /** @brief Retrieves a reference to the generator's value.
     *
     * When the generator is in a suspended state, the value it has yielded
     * lies on it stack, which is guaranteed to be alive. Therefore we don't
     * have to copy the value and can return a cheap reference to it.
     *
     * @throws ostd::coroutine_error if the generator is dead.
     */
    T const &value() const {
        if (!p_result) {
            throw coroutine_error{"no value"};
        }
        return *p_result;
    }

    /** Checks if the generator is has a value, returning `false` if it does. */
    bool empty() const noexcept {
        return !p_result;
    }

    /** @brief Gets a range to the generator.
     *
     * The range is an ostd::generator_range<T>. It's an input range, so all
     * of its copies point to the same generator instance, reflecting the same
     * internal state.
     */
    generator_range<T> iter() noexcept;

    /** Implements a minimal iterator just for range-based for loop.
      * Do not use directly.
      */
    detail::generator_iterator<T> begin() noexcept;

    /** Implements a minimal iterator just for range-based for loop.
      * Do not use directly.
      */
    std::nullptr_t end() noexcept {
        return nullptr;
    }

    /** Swaps two generators' states. */
    void swap(generator &other) noexcept {
        using std::swap;
        swap(p_func, other.p_func);
        swap(p_result, other.p_result);
        coroutine_context::swap(other);
    }

    /** Returns the RTTI of the function stored in the generator. */
    std::type_info const &target_type() const {
        return p_func.target_type();
    }

    /** @brief Retrieves the stored function given its known type.
     *
     * @returns A pointer to the function or `nullptr` if the type
     *          doesn't match.
     */
    template<typename F>
    F *target() { return p_func.target(); }

    /** @brief Retrieves the stored function given its known type.
     *
     * @returns A pointer to the function or `nullptr` if the type
     *          doesn't match.
     */
    template<typename F>
    F const *target() const { return p_func.target(); }

private:
    void resume_call() {
        p_func(yield_type{*this});
        /* done, gotta null the item so that empty() returns true */
        p_result = nullptr;
    }

    std::function<void(yield_type)> p_func;
    /* we can use a pointer because even stack values are alive
     * as long as the coroutine is alive (and it is on every yield)
     */
    std::remove_reference_t<T> *p_result = nullptr;
};

/** Swaps two generators' states. */
template<typename T>
inline void swap(generator<T> &a, generator<T> &b) noexcept {
    a.swap(b);
}

namespace detail {
    template<typename T>
    struct yield_type_base {
        using type = typename generator<T>::yield_type;
    };
    template<typename R, typename ...A>
    struct yield_type_base<R(A...)> {
        using type = typename coroutine<R(A...)>::yield_type;
    };
}

/** @brief Gets the yielder type for the given type.
 *
 * If the given type is like `R(A...)`, the resulting type is an yielder type
 * for the matching ostd::coroutine. If it's just a simple type, it's an
 * yielder type for a generator of that type.
 */
template<typename T>
using yield_type = typename detail::yield_type_base<T>::type;

/** @brief A range type over a generator.
 *
 * This range type is a simple input range type (ostd::input_range_tag)
 * that wraps around a generator.
 */
template<typename T>
struct generator_range: input_range<generator_range<T>> {
    using range_category  = input_range_tag;
    using value_type      = T;
    using reference       = T &;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;

    /** Generator ranges are not default constructible. */
    generator_range() = delete;

    /** Generator ranges are constructed using a reference to a generator. */
    generator_range(generator<T> &g): p_gen(&g) {}

    /** Like ostd::generator<T>::empty(). */
    bool empty() const noexcept {
        return p_gen->empty();
    }

    /** Like ostd::generator<T>::resume(). */
    void pop_front() {
        p_gen->resume();
    }

    /** Like ostd::generator<T>::value(). */
    reference front() const {
        return p_gen->value();
    }

    /** Simply compares the two stored generator references for equality. */
    bool equals_front(generator_range const &g) const noexcept {
        return p_gen == g.p_gen;
    }

    /** Implements a minimal iterator just for range-based for loop.
      * Do not use directly.
      */
    detail::generator_iterator<T> begin() noexcept;

    /** Implements a minimal iterator just for range-based for loop.
      * Do not use directly.
      */
    std::nullptr_t end() noexcept {
        return nullptr;
    }

private:
    generator<T> *p_gen;
};

template<typename T>
generator_range<T> generator<T>::iter() noexcept {
    return generator_range<T>{*this};
}

namespace detail {
    /* deliberately incomplete, only for range for loop */
    template<typename T>
    struct generator_iterator {
        generator_iterator() = delete;
        generator_iterator(generator<T> &g): p_gen(&g) {}

        bool operator!=(std::nullptr_t) noexcept {
            return !p_gen->empty();
        }

        T &operator*() const {
            return p_gen->value();
        }

        generator_iterator &operator++() {
            p_gen->resume();
            return *this;
        }

    private:
        generator<T> *p_gen;
    };
} /* namespace detail */

template<typename T>
detail::generator_iterator<T> generator<T>::begin() noexcept {
    return detail::generator_iterator<T>{*this};
}

template<typename T>
detail::generator_iterator<T> generator_range<T>::begin() noexcept {
    return detail::generator_iterator<T>{*p_gen};
}

/** @} */

} /* namespace ostd */

#endif

/** @} */
