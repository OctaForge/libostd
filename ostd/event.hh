/* signals/slots for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_EVENT_HH
#define OSTD_EVENT_HH

#include <functional>
#include <utility>

namespace ostd {

namespace detail {
    template<typename C, typename ...A>
    struct signal_base {
        signal_base(C *cl): p_class(cl), p_funcs(nullptr), p_nfuncs(0) {}

        signal_base(signal_base const &ev):
            p_class(ev.p_class), p_nfuncs(ev.p_nfuncs)
        {
            using func_t = std::function<void(C &, A...)>;
            auto *bufp = new unsigned char[sizeof(func_t) * p_nfuncs];
            func_t *nbuf = reinterpret_cast<func_t *>(bufp);
            for (size_t i = 0; i < p_nfuncs; ++i)
                new (&nbuf[i]) func_t(ev.p_funcs[i]);
            p_funcs = nbuf;
        }

        signal_base(signal_base &&ev):
            p_class(nullptr), p_funcs(nullptr), p_nfuncs(0)
        {
            swap(ev);
        }

        signal_base &operator=(signal_base const &ev) {
            using func_t = std::function<void(C &, A...)>;
            p_class = ev.p_class;
            p_nfuncs = ev.p_nfuncs;
           auto *bufp = new unsigned char[sizeof(func_t) * p_nfuncs];
            func_t *nbuf = reinterpret_cast<func_t *>(bufp);
            for (size_t i = 0; i < p_nfuncs; ++i) {
                new (&nbuf[i]) func_t(ev.p_funcs[i]);
            }
            p_funcs = nbuf;
            return *this;
        }

        signal_base &operator=(signal_base &&ev) {
            swap(ev);
            return *this;
        }

        ~signal_base() { clear(); }

        void clear() {
            for (size_t i = 0; i < p_nfuncs; ++i) {
                using func = std::function<void(C &, A...)>;
                p_funcs[i].~func();
            }
            delete[] reinterpret_cast<unsigned char *>(p_funcs);
            p_funcs = nullptr;
            p_nfuncs = 0;
        }

        template<typename F>
        size_t connect(F &&func) {
            using func_t = std::function<void(C &, A...)>;
            for (size_t i = 0; i < p_nfuncs; ++i) {
                if (!p_funcs[i]) {
                    p_funcs[i] = std::forward<F>(func);
                    return i;
                }
            }
            auto *bufp = new unsigned char[sizeof(func_t) * (p_nfuncs + 1)];
            func_t *nbuf = reinterpret_cast<func_t *>(bufp);
            for (size_t i = 0; i < p_nfuncs; ++i) {
                new (&nbuf[i]) func_t(std::move(p_funcs[i]));
                p_funcs[i].~func_t();
            }
            new (&nbuf[p_nfuncs]) func_t(std::forward<F>(func));
            delete[] reinterpret_cast<unsigned char *>(p_funcs);
            p_funcs = nbuf;
            return p_nfuncs++;
        }

        bool disconnect(size_t idx) {
            if ((idx >= p_nfuncs) || !p_funcs[idx]) {
                return false;
            }
            p_funcs[idx] = nullptr;
            return true;
        }

        template<typename ...Args>
        void emit(Args &&...args) const {
            if (!p_class) {
                return;
            }
            for (size_t i = 0; i < p_nfuncs; ++i) {
                if (p_funcs[i]) {
                    p_funcs[i](*p_class, args...);
                }
            }
        }

        C *get_class() const {
            return p_class;
        }

        C *set_class(C *cl) {
            C *ocl = p_class;
            p_class = cl;
            return ocl;
        }

        void swap(signal_base &ev) {
            using std::swap;
            swap(p_class, ev.p_class);
            swap(p_funcs, ev.p_funcs);
            swap(p_nfuncs, ev.p_nfuncs);
        }

    private:
        C *p_class;
        std::function<void(C &, A...)> *p_funcs;
        size_t p_nfuncs;
    };
} /* namespace detail */

template<typename C, typename ...A>
struct signal {
private:
    using base_t = detail::signal_base<C, A...>;
    base_t p_base;
public:
    signal(C *cl): p_base(cl) {}
    signal(signal const &ev): p_base(ev.p_base) {}
    signal(signal &&ev): p_base(std::move(ev.p_base)) {}

    signal &operator=(signal const &ev) {
        p_base = ev.p_base;
        return *this;
    }

    signal &operator=(signal &&ev) {
        p_base = std::move(ev.p_base);
        return *this;
    }

    void clear() { p_base.clear(); }

    template<typename F>
    size_t connect(F &&func) { return p_base.connect(std::forward<F>(func)); }

    bool disconnect(size_t idx) { return p_base.disconnect(idx); }

    template<typename ...Args>
    void emit(Args &&...args) { p_base.emit(std::forward<Args>(args)...); }

    template<typename ...Args>
    void operator()(Args &&...args) { emit(std::forward<Args>(args)...); }

    C *get_class() const { return p_base.get_class(); }
    C *set_class(C *cl) { return p_base.set_class(cl); }

    void swap(signal &ev) { p_base.swap(ev.p_base); }
};

template<typename C, typename ...A>
struct signal<C const, A...> {
private:
    using base_t = detail::signal_base<C const, A...>;
    base_t p_base;
public:
    signal(C *cl): p_base(cl) {}
    signal(signal const &ev): p_base(ev.p_base) {}
    signal(signal &&ev): p_base(std::move(ev.p_base)) {}

    signal &operator=(signal const &ev) {
        p_base = ev.p_base;
        return *this;
    }

    signal &operator=(signal &&ev) {
        p_base = std::move(ev.p_base);
        return *this;
    }

    void clear() { p_base.clear(); }

    template<typename F>
    size_t connect(F &&func) { return p_base.connect(std::forward<F>(func)); }

    bool disconnect(size_t idx) { return p_base.disconnect(idx); }

    template<typename ...Args>
    void emit(Args &&...args) const { p_base.emit(std::forward<Args>(args)...); }

    template<typename ...Args>
    void operator()(Args &&...args) const { emit(std::forward<Args>(args)...); }

    C *get_class() const { return p_base.get_class(); }
    C *set_class(C *cl) { return p_base.set_class(cl); }

    void swap(signal &ev) { p_base.swap(ev.p_base); }
};

template<typename C, typename ...A>
inline void swap(signal<C, A...> &a, signal<C, A...> &b) {
    a.swap(b);
}

} /* namespace ostd */

#endif
