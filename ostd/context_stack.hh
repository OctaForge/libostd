/* Stack allocation for coroutine contexts.
 * API more or less compatible with the Boost.Context library.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_CONTEXT_STACK_HH
#define OSTD_CONTEXT_STACK_HH

#include <new>
#include <algorithm>

#include "ostd/types.hh"
#include "ostd/platform.hh"

#if !defined(OSTD_PLATFORM_POSIX) && !defined(OSTD_PLATFORM_WIN32)
#  error "Unsupported platform"
#endif

#ifdef OSTD_USE_VALGRIND
#  include <valgrind/valgrind.h>
#endif

namespace ostd {

struct stack_context {
    void *ptr = nullptr;
    size_t size = 0;
#ifdef OSTD_USE_VALGRIND
    int valgrind_id = 0;
#endif
};

struct OSTD_EXPORT stack_traits {
    static bool is_unbounded() noexcept;
    static size_t page_size() noexcept;
    static size_t minimum_size() noexcept;
    static size_t maximum_size() noexcept;
    static size_t default_size() noexcept;
};

namespace detail {
    OSTD_EXPORT void *stack_alloc(size_t sz);
    OSTD_EXPORT void stack_free(void *p, size_t sz) noexcept;
    OSTD_EXPORT void stack_protect(void *p, size_t sz) noexcept;
}

template<typename TR, bool Protected>
struct basic_fixedsize_stack {
    using traits_type = TR;

    basic_fixedsize_stack(size_t ss = TR::default_size()) noexcept:
        p_size(std::clamp(ss, TR::minimum_size(), TR::maximum_size()))
    {}

    stack_context allocate() {
        size_t ss = p_size;

        size_t pgs = TR::page_size();
        size_t npg = std::max(ss / pgs, size_t(size_t(Protected) + 1));
        size_t asize = npg * pgs;

        void *p = detail::stack_alloc(asize);
        if constexpr(Protected) {
            /* a single guard page */
            detail::stack_protect(p, pgs);
        }

        stack_context ret{static_cast<byte *>(p) + ss, ss};
#ifdef OSTD_USE_VALGRIND
        ret.valgrind_id = VALGRIND_STACK_REGISTER(ret.ptr, p);
#endif
        return ret;
    }

    void deallocate(stack_context &st) noexcept {
        if (!st.ptr) {
            return;
        }
#ifdef OSTD_USE_VALGRIND
        VALGRIND_STACK_DEREGISTER(st.valgrind_id);
#endif
        detail::stack_free(static_cast<byte *>(st.ptr) - st.size, st.size);
        st.ptr = nullptr;
    }

private:
    size_t p_size;
};

using fixedsize_stack = basic_fixedsize_stack<stack_traits, false>;
using protected_fixedsize_stack = basic_fixedsize_stack<stack_traits, true>;

template<typename TR, bool Protected>
struct basic_stack_pool {
private:
    struct allocator {
        allocator() = delete;
        allocator(basic_stack_pool &p) noexcept: p_pool(&p) {}

        stack_context allocate() {
            return p_pool->allocate();
        }

        void deallocate(stack_context &st) noexcept {
            p_pool->deallocate(st);
        }

    private:
        basic_stack_pool *p_pool;
    };

public:
    static constexpr size_t DEFAULT_CHUNK_SIZE = 32;

    using allocator_type = allocator;

    basic_stack_pool(
        size_t ss = TR::default_size(), size_t cs = DEFAULT_CHUNK_SIZE
    ) {
        /* precalculate the sizes */
        size_t pgs = TR::page_size();
        size_t npg = std::max(ss / pgs, size_t(size_t(Protected) + 1));
        size_t asize = npg * pgs;
        p_stacksize = asize;
        p_chunksize = cs * asize;
    }

    basic_stack_pool(basic_stack_pool const &) = delete;
    basic_stack_pool(basic_stack_pool &&p) noexcept {
        swap(p);
    }

    basic_stack_pool &operator=(basic_stack_pool const &) = delete;
    basic_stack_pool &operator=(basic_stack_pool &&p) noexcept {
        swap(p);
        return *this;
    }

    ~basic_stack_pool() {
        size_t ss = p_stacksize;
        size_t cs = p_chunksize;
        void *pc = p_chunk;
        while (pc) {
            void *p = pc;
            pc = get_node(p, ss, 1)->next_chunk;
            detail::stack_free(p, cs);
        }
    }

    stack_context allocate() {
        stack_node *nd = request();
        size_t ss = p_stacksize - sizeof(stack_node);
        auto *p = reinterpret_cast<unsigned char *>(nd) - ss;
        if constexpr(Protected) {
            detail::stack_protect(p, TR::page_size());
        }
        stack_context ret{nd, ss};
#ifdef OSTD_USE_VALGRIND
        ret.valgrind_id = VALGRIND_STACK_REGISTER(ret.ptr, p);
#endif
        return ret;
    }

    void deallocate(stack_context &st) noexcept {
        if (!st.ptr) {
            return;
        }
#ifdef OSTD_USE_VALGRIND
        VALGRIND_STACK_DEREGISTER(st.valgrind_id);
#endif
        stack_node *nd = static_cast<stack_node *>(st.ptr);
        stack_node *unused = p_unused;
        nd->next = unused;
        p_unused = nd;
    }

    void swap(basic_stack_pool &p) noexcept {
        using std::swap;
        swap(p_chunk, p.p_chunk);
        swap(p_unused, p.p_unused);
        swap(p_chunksize, p.p_chunksize);
        swap(p_stacksize, p.p_stacksize);
    }

    allocator_type get_allocator() noexcept {
        return allocator{*this};
    }

private:
    struct stack_node {
        void *next_chunk;
        void *next;
    };

    stack_node *request() {
        if (!p_unused) {
            size_t ss = p_stacksize;
            size_t cs = p_chunksize;
            size_t cnum = cs / ss;

            void *chunk = detail::stack_alloc(cs);
            void *prevn = nullptr;
            for (size_t i = cnum; i >= 2; --i) {
                auto *nd = get_node(chunk, ss, i);
                nd->next_chunk = nullptr;
                nd->next = prevn;
                prevn = nd;
            }
            auto *fnd = get_node(chunk, ss, 1);
            fnd->next_chunk = p_chunk;
            p_chunk = chunk;
            fnd->next = prevn;
            p_unused = fnd;
        }
        stack_node *r = p_unused;
        p_unused = static_cast<stack_node *>(r->next);
        return r;
    }

    stack_node *get_node(void *chunk, size_t ssize, size_t n) {
        return reinterpret_cast<stack_node *>(
            static_cast<unsigned char *>(chunk) + (ssize * n) - sizeof(stack_node)
        );
    }

    void *p_chunk = nullptr;
    stack_node *p_unused = nullptr;

    size_t p_chunksize;
    size_t p_stacksize;
};

template<typename TR, bool P>
inline void swap(basic_stack_pool<TR, P> &a, basic_stack_pool<TR, P> &b) noexcept {
    a.swap(b);
}

using stack_pool = basic_stack_pool<stack_traits, false>;
using protected_stack_pool = basic_stack_pool<stack_traits, true>;

using default_stack = fixedsize_stack;

} /* namespace ostd */

#endif
