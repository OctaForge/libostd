/* Signals/slots for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_EVENT_HH
#define OSTD_EVENT_HH

#include "ostd/functional.hh"
#include "ostd/utility.hh"

namespace ostd {

namespace detail {
    template<typename C, typename ...A>
    struct SignalBase {
        SignalBase(C *cl): p_class(cl), p_funcs(nullptr), p_nfuncs(0) {}

        SignalBase(SignalBase const &ev):
            p_class(ev.p_class), p_nfuncs(ev.p_nfuncs)
        {
            using func_t = std::function<void(C &, A...)>;
            byte *bufp = new byte[sizeof(func_t) * p_nfuncs];
            func_t *nbuf = reinterpret_cast<func_t *>(bufp);
            for (Size i = 0; i < p_nfuncs; ++i)
                new (&nbuf[i]) func_t(ev.p_funcs[i]);
            p_funcs = nbuf;
        }

        SignalBase(SignalBase &&ev):
            p_class(nullptr), p_funcs(nullptr), p_nfuncs(0)
        {
            swap(ev);
        }

        SignalBase &operator=(SignalBase const &ev) {
            using func_t = std::function<void(C &, A...)>;
            p_class = ev.p_class;
            p_nfuncs = ev.p_nfuncs;
            byte *bufp = new byte[sizeof(func_t) * p_nfuncs];
            func_t *nbuf = reinterpret_cast<func_t *>(bufp);
            for (Size i = 0; i < p_nfuncs; ++i) {
                new (&nbuf[i]) func_t(ev.p_funcs[i]);
            }
            p_funcs = nbuf;
            return *this;
        }

        SignalBase &operator=(SignalBase &&ev) {
            swap(ev);
            return *this;
        }

        ~SignalBase() { clear(); }

        void clear() {
            for (Size i = 0; i < p_nfuncs; ++i) {
                using func = std::function<void(C &, A...)>;
                p_funcs[i].~func();
            }
            delete[] reinterpret_cast<byte *>(p_funcs);
            p_funcs = nullptr;
            p_nfuncs = 0;
        }

        template<typename F>
        Size connect(F &&func) {
            using func_t = std::function<void(C &, A...)>;
            for (Size i = 0; i < p_nfuncs; ++i) {
                if (!p_funcs[i]) {
                    p_funcs[i] = std::forward<F>(func);
                    return i;
                }
            }
            byte *bufp = new byte[sizeof(func_t) * (p_nfuncs + 1)];
            func_t *nbuf = reinterpret_cast<func_t *>(bufp);
            for (Size i = 0; i < p_nfuncs; ++i) {
                new (&nbuf[i]) func_t(std::move(p_funcs[i]));
                p_funcs[i].~func_t();
            }
            new (&nbuf[p_nfuncs]) func_t(std::forward<F>(func));
            delete[] reinterpret_cast<byte *>(p_funcs);
            p_funcs = nbuf;
            return p_nfuncs++;
        }

        bool disconnect(Size idx) {
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
            for (Size i = 0; i < p_nfuncs; ++i) {
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

        void swap(SignalBase &ev) {
            using std::swap;
            swap(p_class, ev.p_class);
            swap(p_funcs, ev.p_funcs);
            swap(p_nfuncs, ev.p_nfuncs);
        }

    private:
        C *p_class;
        std::function<void(C &, A...)> *p_funcs;
        Size p_nfuncs;
    };
} /* namespace detail */

template<typename C, typename ...A>
struct Signal {
private:
    using Base = detail::SignalBase<C, A...>;
    Base p_base;
public:
    Signal(C *cl): p_base(cl) {}
    Signal(Signal const &ev): p_base(ev.p_base) {}
    Signal(Signal &&ev): p_base(std::move(ev.p_base)) {}

    Signal &operator=(Signal const &ev) {
        p_base = ev.p_base;
        return *this;
    }

    Signal &operator=(Signal &&ev) {
        p_base = std::move(ev.p_base);
        return *this;
    }

    void clear() { p_base.clear(); }

    template<typename F>
    Size connect(F &&func) { return p_base.connect(std::forward<F>(func)); }

    bool disconnect(Size idx) { return p_base.disconnect(idx); }

    template<typename ...Args>
    void emit(Args &&...args) { p_base.emit(std::forward<Args>(args)...); }

    template<typename ...Args>
    void operator()(Args &&...args) { emit(std::forward<Args>(args)...); }

    C *get_class() const { return p_base.get_class(); }
    C *set_class(C *cl) { return p_base.set_class(cl); }

    void swap(Signal &ev) { p_base.swap(ev.p_base); }
};

template<typename C, typename ...A>
struct Signal<C const, A...> {
private:
    using Base = detail::SignalBase<C const, A...>;
    Base p_base;
public:
    Signal(C *cl): p_base(cl) {}
    Signal(Signal const &ev): p_base(ev.p_base) {}
    Signal(Signal &&ev): p_base(std::move(ev.p_base)) {}

    Signal &operator=(Signal const &ev) {
        p_base = ev.p_base;
        return *this;
    }

    Signal &operator=(Signal &&ev) {
        p_base = std::move(ev.p_base);
        return *this;
    }

    void clear() { p_base.clear(); }

    template<typename F>
    Size connect(F &&func) { return p_base.connect(std::forward<F>(func)); }

    bool disconnect(Size idx) { return p_base.disconnect(idx); }

    template<typename ...Args>
    void emit(Args &&...args) const { p_base.emit(std::forward<Args>(args)...); }

    template<typename ...Args>
    void operator()(Args &&...args) const { emit(std::forward<Args>(args)...); }

    C *get_class() const { return p_base.get_class(); }
    C *set_class(C *cl) { return p_base.set_class(cl); }

    void swap(Signal &ev) { p_base.swap(ev.p_base); }
};

template<typename C, typename ...A>
inline void swap(Signal<C, A...> &a, Signal<C, A...> &b) {
    a.swap(b);
}

} /* namespace ostd */

#endif
