/* Thread support library.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_THREAD_HH
#define OSTD_THREAD_HH

#include <stdlib.h>
#include <threads.h>

#include "ostd/memory.hh"
#include "ostd/platform.hh"
#include "ostd/type_traits.hh"
#include "ostd/tuple.hh"

namespace ostd {

struct Thread;

namespace detail {
    struct ThreadId;
}

namespace this_thread {
    inline ostd::detail::ThreadId get_id();
}

namespace detail {
    struct ThreadId {
        ThreadId(): p_thread(0) {}

        friend bool operator==(ThreadId a, ThreadId b) {
            return a.p_thread == b.p_thread;
        }

        friend bool operator!=(ThreadId a, ThreadId b) {
            return !(a == b);
        }

        friend bool operator<(ThreadId a, ThreadId b) {
            return a.p_thread < b.p_thread;
        }

        friend bool operator<=(ThreadId a, ThreadId b) {
            return !(b < a);
        }

        friend bool operator>(ThreadId a, ThreadId b) {
            return b < a;
        }

        friend bool operator>=(ThreadId a, ThreadId b) {
            return !(a < b);
        }
    private:
        ThreadId(thrd_t t): p_thread(t) {}

        friend struct ostd::Thread;
        friend ThreadId ostd::this_thread::get_id();

        thrd_t p_thread;
    };
}

namespace this_thread {
    inline ostd::detail::ThreadId get_id() {
        return thrd_current();
    }

    inline void yield() {
        thrd_yield();
    }
}

namespace detail {
    template<typename T>
    inline Decay<T> decay_copy(T &&v) {
        return forward<T>(v);
    }

    template<typename F, typename ...A, Size ...I>
    inline void thread_exec(Tuple<F, A...> &tup, detail::TupleIndices<I...>) {
        ostd::get<0>(tup)(ostd::move(ostd::get<I>(tup))...);
    }

    template<typename F>
    inline int thread_proxy(void *ptr) {
        Box<F> fptr((F *)ptr);
        using Index = detail::MakeTupleIndices<TupleSize<F>, 1>;
        detail::thread_exec(*fptr, Index());
        return 0;
    }
}

struct Thread {
    using NativeHandle = thrd_t;

    Thread(): p_thread(0) {}
    Thread(Thread &&o): p_thread(o.p_thread) { o.p_thread = 0; }

    template<typename F, typename ...A,
             typename = EnableIf<!IsSame<Decay<F>, Thread>>>
    Thread(F &&func, A &&...args) {
        using FuncT = Tuple<Decay<F>, Decay<A>...>;
        Box<FuncT> p(new FuncT(detail::decay_copy(forward<F>(func)),
                               detail::decay_copy(forward<A>(args))...));
        int res = thrd_create(&p_thread, &detail::thread_proxy<FuncT>, p.get());
        if (res == thrd_success)
            p.release();
        else
            p_thread = 0;
    }

    Thread &operator=(Thread &&other) {
        if (joinable())
            abort();
        p_thread = other.p_thread;
        other.p_thread = 0;
        return *this;
    }

    ~Thread() {
        if (joinable())
            abort();
    }

    explicit operator bool() const { return joinable(); }
    bool joinable() const { return p_thread != 0; }

    NativeHandle native_handle() { return p_thread; }

    detail::ThreadId get_id() {
        return p_thread;
    }

    bool join() {
        if (!joinable())
            return false;
        thrd_t cur = p_thread;
        p_thread = 0;
        return thrd_join(cur, nullptr) == thrd_success;
    }

    bool detach() {
        if (!joinable())
            return false;
        if (thrd_detach(p_thread) == thrd_success) {
            p_thread = 0;
            return true;
        }
        return false;
    }

    void swap(Thread &other) {
        thrd_t cur = p_thread;
        p_thread = other.p_thread;
        other.p_thread = cur;
    }

private:
    thrd_t p_thread;
};

} /* namespace ostd */

#endif