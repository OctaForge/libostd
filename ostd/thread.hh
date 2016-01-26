/* Thread support library.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_THREAD_HH
#define OSTD_THREAD_HH

#include <stdlib.h>
#include <pthread.h>

#ifndef OSTD_PLATFORM_WIN32
#include <unistd.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

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
        ThreadId(pthread_t t): p_thread(t) {}

        friend struct ostd::Thread;
        friend ThreadId ostd::this_thread::get_id();

        pthread_t p_thread;
    };
}

namespace this_thread {
    inline ostd::detail::ThreadId get_id() {
        return pthread_self();
    }

    inline void yield() {
        sched_yield();
    }

    inline void exit() {
        pthread_exit(nullptr);
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
    inline void *thread_proxy(void *ptr) {
        Box<F> fptr((F *)ptr);
        using Index = detail::MakeTupleIndices<TupleSize<F>, 1>;
        detail::thread_exec(*fptr, Index());
        return nullptr;
    }
}

struct Thread {
    using NativeHandle = pthread_t;

    Thread(): p_thread(0) {}
    Thread(Thread &&o): p_thread(o.p_thread) { o.p_thread = 0; }

    template<typename F, typename ...A,
             typename = EnableIf<!IsSame<Decay<F>, Thread>>>
    Thread(F &&func, A &&...args) {
        using FuncT = Tuple<Decay<F>, Decay<A>...>;
        Box<FuncT> p(new FuncT(detail::decay_copy(forward<F>(func)),
                               detail::decay_copy(forward<A>(args))...));
        int res = pthread_create(&p_thread, 0, &detail::thread_proxy<FuncT>, p.get());
        if (!res)
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
        auto ret = pthread_join(p_thread, nullptr);
        p_thread = 0;
        return !ret;
    }

    bool detach() {
        bool ret = false;
        if (p_thread)
            ret = !pthread_detach(p_thread);
        p_thread = 0;
        return ret;
    }

    void swap(Thread &other) {
        auto cur = p_thread;
        p_thread = other.p_thread;
        other.p_thread = cur;
    }

    static ostd::uint hardware_concurrency() {
        static ostd::uint count = 0;
        if (count <= 0) {
#ifdef OSTD_PLATFORM_WIN32
            SYSTEM_INFO info;
            GetSystemInfo(&info);
            count = info.dwNumberOfProcessors;
#elif defined(_SC_NPROCESSORS_ONLN)
            count = ostd::uint(sysconf(_SC_NPROCESSORS_ONLN));
#endif
            if (count <= 0)
                count = 1;
        }
        return count;
    }

private:
    pthread_t p_thread;
};

} /* namespace ostd */

#endif