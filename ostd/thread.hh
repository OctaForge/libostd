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
    template<typename F>
    inline int thread_proxy(void *ptr) {
        Box<F> fptr((F *)ptr);
        (*fptr)();
        return 0;
    }
}

struct Thread {
    using NativeHandle = thrd_t;

    Thread(): p_thread(0) {}
    Thread(Thread &&o): p_thread(o.p_thread) { o.p_thread = 0; }

    template<typename F>
    Thread(F &&func) {
        Box<F> p(new F(func));
        int res = thrd_create(&p_thread, &detail::thread_proxy<F>, p.get());
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

    operator bool() const { return joinable(); }
    bool joinable() const { return p_thread != 0; }

    NativeHandle native_handle() const { return p_thread; }

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