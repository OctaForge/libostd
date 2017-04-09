/** @addtogroup Concurrency
 * @{
 */

/** @file context_stack.hh
 *
 * @brief Stack allocation for coroutines and other context-using types.
 *
 * Contexts, which are used by coroutines, generators and tasks in certain
 * concurrency scheduler types, need stacks to work. This file provides
 * several types of stack allocators to suit their needs.
 *
 * This API is mostly inspired by Boost.Context stack API.
 *
 * @copyright See COPYING.md in the project tree for further information.
 */

#ifndef OSTD_CONTEXT_STACK_HH
#define OSTD_CONTEXT_STACK_HH

#include <cstddef>
#include <new>
#include <algorithm>

#include "ostd/platform.hh"

#if !defined(OSTD_PLATFORM_POSIX) && !defined(OSTD_PLATFORM_WIN32)
#  error "Unsupported platform"
#endif

#ifdef OSTD_USE_VALGRIND
#  include <valgrind/valgrind.h>
#endif

namespace ostd {

/** @addtogroup Concurrency
 * @{
 */

/** @brief An allocated stack.
 *
 * This represents a stack allocated by a stack allocator. It doesn't
 * release itself so it has to be deallocated using the same stack allocator.
 *
 * On architectures where the stack grows down, the stack pointer is actually
 * `allocated_memory + stack_size`. This is the majority of architectures and
 * all architectures this module supports.
 */
struct stack_context {
    void *ptr = nullptr; ///< The stack pointer.
    std::size_t size = 0; ///< The stack size.
#ifdef OSTD_USE_VALGRIND
    int valgrind_id = 0;
#endif
};

/** @brief The stack traits to check properties of stacks.
 *
 * This structure allows stack allocators (and potentially others) to check
 * various properties of stacks on your system, mainly sizing-wise.
 */
struct OSTD_EXPORT stack_traits {
    /** @brief Checks if the stack is unbounded.
     *
     * This checks whether the stack is limited in size. If it's not, it
     * means the stack you can allocate can take up as much memory as you
     * want. Otherwise it means that there is some limit to it, which you
     * can query with maximum_size().
     */
    static bool is_unbounded() noexcept;

    /** @brief Gets the page size on your system.
     *
     * The returned size is in bytes. Typically this is 4 KiB, but can be
     * some other value too. Stack sizes are typically a multiple of page
     * size.
     */
    static std::size_t page_size() noexcept;

    /** @brief Gets the minimum size a stack can have.
     *
     * You need at least this for coroutine stacks. On POSIX systems, this
     * typically defaults to `SIGSTKSZ`. On Windows, we specify 8 KiB as
     * the minimum size. The returned size is in bytes.
     *
     * @see maximum_size(), default_size()
     */
    static std::size_t minimum_size() noexcept;

    /** @brief Gets the maximum size a stack can have.
     *
     * Please remember that if is_unbounded() returns `true`, the result
     * of claling this is undefined. Therefore, only use this if there is
     * a stack limit in place. The returned size is in bytes.
     *
     * @see minimum_size(), default_size()
     */
    static std::size_t maximum_size() noexcept;

    /** @brief Gets the default size for stacks.
     *
     * Returns a reasonable size for a coroutine stack (not the main stack).
     * Currently this is set to be 64 KiB, unless minimum_size() returns a
     * bigger value (in which case it's the result of minimum_size()).
     *
     * @see minimum_size(), maximum_size()
     */
    static std::size_t default_size() noexcept;
};

namespace detail {
    OSTD_EXPORT void *stack_alloc(std::size_t sz);
    OSTD_EXPORT void stack_free(void *p, std::size_t sz) noexcept;
    OSTD_EXPORT void stack_protect(void *p, std::size_t sz) noexcept;
    OSTD_EXPORT std::size_t stack_main_size() noexcept;
}

/** @brief A fixed size stack.
 *
 * A normal stack with a fixed size. The size of stacks alloated by this
 * is always at least one page. Protected stacks add an extra guard page
 * to the end, which is not usable by the stack. The size of the stack
 * is always a multiple of page size. If the requested size is not a
 * multiple, it's rounded up to the nearest multiple.
 *
 * System specific facilities are used to allocate the stacks. On POSIX
 * systems, this tends to be `mmap()` with `MAP_ANON` or `MAP_ANONYMOUS`,
 * unless not available (in which case the fallback is just `malloc`).
 * On Windows, `VirtualAlloc()` is used.
 *
 * This allocator can also be used in places a stack pool is necessary,
 * allocating single stacks (not from a pool). See ostd::basic_stack_pool
 * for more information.
 *
 * @tparam Traits The stack traits to use (typically ostd::stack_traits).
 * @tparam Protected Whether to protect the stack.
 */
template<typename Traits, bool Protected>
struct basic_fixedsize_stack {
    /** @brief The traits type used for the stacks. */
    using traits_type = Traits;

    /** @brief The allocator type, here it's the allocator itself.
     *
     * This allows the plain allocator to be used in cases where a stack
     * pool is expected to be used for allocations. See get_allocator().
     */
    using allocator_type = basic_fixedsize_stack;

    /** @brief Fixed size stacks are thread safe by default.
     *
     * You can allocate stacks from multiple threads using the same
     * structure without any locking, there is no mutable shared state.
     */
    static constexpr bool is_thread_safe = true;

    /** @brief Constructs the stack allocator.
     *
     * The provided argument is the size used for the stacks. It defaults
     * to the default size used for the stacks according to the traits.
     */
    basic_fixedsize_stack(std::size_t ss = Traits::default_size()) noexcept:
        p_size(
            Traits::is_unbounded()
                ? std::max(ss, Traits::minimum_size())
                : std::clamp(ss, Traits::minimum_size(), Traits::maximum_size())
        )
    {}

    /** @brief Allocates a stack. */
    stack_context allocate() {
        std::size_t ss = p_size;

        std::size_t pgs = Traits::page_size();
        std::size_t asize = ss + pgs - 1 - (ss - 1) % pgs + (pgs * Protected);

        void *p = detail::stack_alloc(asize);
        if constexpr(Protected) {
            /* a single guard page */
            detail::stack_protect(p, pgs);
        }

        stack_context ret{static_cast<unsigned char *>(p) + asize, asize};
#ifdef OSTD_USE_VALGRIND
        ret.valgrind_id = VALGRIND_STACK_REGISTER(ret.ptr, p);
#endif
        return ret;
    }

    /** @brief Deallocates a stack. */
    void deallocate(stack_context &st) noexcept {
        if (!st.ptr) {
            return;
        }
#ifdef OSTD_USE_VALGRIND
        VALGRIND_STACK_DEREGISTER(st.valgrind_id);
#endif
        detail::stack_free(
            static_cast<unsigned char *>(st.ptr) - st.size, st.size
        );
        st.ptr = nullptr;
    }

    /** @brief A no-op function for stack pool uses.
     *
     * When a stack pool is used to allocate stacks somewhere, multiple
     * stacks can be reserved ahead of time. This allocates only one stack
     * at a time, so this function does nothing.
     */
    void reserve(std::size_t) {}

    /** @brief Gets the allocator for stack pool uses.
     *
     * Since this is not a stack pool, this function returns a copy of
     * this object. This allows the stack allocator to be used as if
     * it was a pool.
     */
    allocator_type get_allocator() noexcept {
        return *this;
    }

private:
    std::size_t p_size;
};

/** @brief An unprotected fixed size stack using ostd::stack_traits. */
using fixedsize_stack = basic_fixedsize_stack<stack_traits, false>;

/** @brief A protected fixed size stack using ostd::stack_traits. */
using protected_fixedsize_stack = basic_fixedsize_stack<stack_traits, true>;

/** @brief A stack pool.
 *
 * A stack pool allocates multiple stacks at a time and gives them out as
 * requested. When the preallocated stacks run out, a new chunk of stacks
 * is allocated. You can also preallocate as many stacks as you want in
 * the pool to prevent allocations from being done at later time.
 *
 * When a stack is "freed" using the pool or a stack allocator requested
 * from the pool, the stack is not actually freed, just returned to the pool
 * and reused next time something else requests a stack.
 *
 * The allocated stacks are fixed size and allocated exactly the same as
 * ostd::basic_fixedsize_stack would.
 *
 * Keep in mind that stack pools are not thread safe, so external locking
 * has to be done (see is_thread_safe).
 *
 * @tparam Traits The stack traits to use (typically ostd::stack_traits).
 * @tparam Protected Whether to protect the stack.
 */
template<typename Traits, bool Protected>
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
    /** @brief The default number of stacks to store in each chunk. */
    static constexpr std::size_t DEFAULT_CHUNK_SIZE = 32;

    /** @brief The traits type used for the stacks. */
    using traits_type = Traits;

    /** @brief The allocator type for the pool.
     *
     * Using get_allocator(), you can request an allocator from the pool
     * which will be just a regular stack allocator except sourcing stacks
     * from the pool. This is the allocator type used for that.
     */
    using allocator_type = allocator;

    /** @brief Stack pools are not thread safe.
     *
     * There is some shared state, so it's necessary to lock when requesting
     * stacks from multiple threads. This is typically not a problem, as in
     * the typical case of doing this some lock needs to be there anyway
     * (typically to handle creation of the objects the stacks will be
     * used in), but sometimes it might be necessary to check this.
     */
    static constexpr bool is_thread_safe = false;

    /** @brief Creates a stack pool.
     *
     * The parameters are optional. The stack size defaults to the default
     * size used for stacks according to the traits. The number of stacks
     * in each chunk defaults to `DEFAULT_CHUNK_SIZE`.
     *
     * @param ss The stack size used for the individual stacks.
     * @param cs The number of stacks in each chunk.
     */
    basic_stack_pool(
        std::size_t ss = Traits::default_size(),
        std::size_t cs = DEFAULT_CHUNK_SIZE
    ) {
        /* precalculate the sizes */
        std::size_t pgs = Traits::page_size();
        std::size_t asize = ss + pgs - 1 - (ss - 1) % pgs + (pgs * Protected);
        p_stacksize = asize;
        p_chunksize = cs * asize;
    }

    /** @brief Stack pools are not copy constructible. */
    basic_stack_pool(basic_stack_pool const &) = delete;

    /** @brief Moves the stack pool.
     *
     * Moves all state from the other pool to this one. The other pool
     * is emptied (no allocated chunks, no capacity) with its stack and
     * chunk sizes remaining the same.
     */
    basic_stack_pool(basic_stack_pool &&p) noexcept {
        p_chunk = p.p_chunk;
        p_unused = p.p_unused;
        p_chunksize = p.p_chunksize;
        p_stacksize = p.p_stacksize;
        p_capacity = p.p_capacity;
        p.p_chunk = nullptr;
        p.p_unused = nullptr;
        p.p_capacity = 0;
    }

    /** @brief Stack pools are not copy assignable. */
    basic_stack_pool &operator=(basic_stack_pool const &) = delete;

    /** @brief Move assigns another pool to this one.
     *
     * Basically performs swap(basic_stack_pool &). If the other pool
     * is not used, the former state of this one is destroyed with it.
     */
    basic_stack_pool &operator=(basic_stack_pool &&p) noexcept {
        swap(p);
        return *this;
    }

    /** @brief Destroys the stack pool and all memory managed by it. */
    ~basic_stack_pool() {
        std::size_t ss = p_stacksize;
        std::size_t cs = p_chunksize;
        void *pc = p_chunk;
        while (pc) {
            void *p = pc;
            pc = get_node(p, ss, 1)->next_chunk;
            detail::stack_free(p, cs);
        }
    }

    /** @brief Reserves a number of stacks.
     *
     * The given number is the actual number of stacks the pool is supposed
     * to contain. If it already contains that number or more, this function
     * does nothing. Otherwise it potentially reserves some extra chunks.
     */
    void reserve(std::size_t n) {
        std::size_t cap = p_capacity;
        if (n <= cap) {
            return;
        }
        std::size_t cnum = p_chunksize / p_stacksize;
        p_unused = alloc_chunks(p_unused, (n - cap + cnum - 1) / cnum);
    }

    /** @brief Requests a stack directly from the pool.
     *
     * As stack allocators are copyable and pools are not, this might not
     * be prectical in most cases. Instead, the pool will be stored somewhere
     * and allocations from it will be done via get_allocator().
     */
    stack_context allocate() {
        stack_node *nd = request();
        std::size_t ss = p_stacksize - sizeof(stack_node);
        auto *p = reinterpret_cast<unsigned char *>(nd) - ss;
        if constexpr(Protected) {
            detail::stack_protect(p, Traits::page_size());
        }
        stack_context ret{nd, ss};
#ifdef OSTD_USE_VALGRIND
        ret.valgrind_id = VALGRIND_STACK_REGISTER(ret.ptr, p);
#endif
        return ret;
    }

    /** @brief Returns a stack back to the pool.
     *
     * This returns the given stack back to the pool for reuse. Stack pool
     * only frees all of its memory when it's destroyed.
     */
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

    /** @brief Swaps two stack pools. */
    void swap(basic_stack_pool &p) noexcept {
        using std::swap;
        swap(p_chunk, p.p_chunk);
        swap(p_unused, p.p_unused);
        swap(p_chunksize, p.p_chunksize);
        swap(p_stacksize, p.p_stacksize);
        swap(p_capacity, p.p_capacity);
    }

    /** @brief Gets a stack allocator that uses the pool.
     *
     * The returned allocator will use allocate() and
     * deallocate(stack_context &) to manage the stacks.
     * You will typically want to use this wherever pools are used.
     */
    allocator_type get_allocator() noexcept {
        return allocator{*this};
    }

private:
    struct stack_node {
        void *next_chunk;
        stack_node *next;
    };

    stack_node *alloc_chunks(stack_node *un, std::size_t n) {
        std::size_t ss = p_stacksize;
        std::size_t cs = p_chunksize;
        std::size_t cnum = cs / ss;

        for (std::size_t ci = 0; ci < n; ++ci) {
            void *chunk = detail::stack_alloc(cs);
            stack_node *prevn = un;
            for (std::size_t i = cnum; i >= 2; --i) {
                auto nd = get_node(chunk, ss, i);
                nd->next_chunk = nullptr;
                nd->next = prevn;
                prevn = nd;
            }
            auto *fnd = get_node(chunk, ss, 1);
            fnd->next_chunk = p_chunk;
            /* write every time so that a potential failure results
             * in all previously allocated chunks being freed in dtor
             */
            p_chunk = chunk;
            fnd->next = prevn;
            un = fnd;
        }
        p_capacity += (n * cnum);

        return un;
    }

    stack_node *request() {
        stack_node *r = p_unused;
        if (!r) {
            r = alloc_chunks(nullptr, 1);
        }
        p_unused = r->next;
        return r;
    }

    stack_node *get_node(void *chunk, std::size_t ssize, std::size_t n) {
        return reinterpret_cast<stack_node *>(
            static_cast<unsigned char *>(chunk) + (ssize * n) - sizeof(stack_node)
        );
    }

    void *p_chunk = nullptr;
    stack_node *p_unused = nullptr;

    std::size_t p_chunksize;
    std::size_t p_stacksize;
    std::size_t p_capacity = 0;
};

/** @brief Swaps two stack pools. */
template<typename Traits, bool P>
inline void swap(
    basic_stack_pool<Traits, P> &a, basic_stack_pool<Traits, P> &b
) noexcept {
    a.swap(b);
}

/** @brief An unprotected stack pool using ostd::stack_traits. */
using stack_pool = basic_stack_pool<stack_traits, false>;

/** @brief A protected stack pool using ostd::stack_traits. */
using protected_stack_pool = basic_stack_pool<stack_traits, true>;

/** @brief The default stack allocator to use when none is provided. */
using default_stack = fixedsize_stack;

/** @} */

} /* namespace ostd */

#endif

/** @} */
