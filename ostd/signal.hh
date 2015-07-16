/* Signals/slots for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OSTD_SIGNAL_HH
#define OSTD_SIGNAL_HH

#include "ostd/functional.hh"
#include "ostd/utility.hh"

namespace ostd {

template<typename C, typename ...A>
struct Event {
    Event(C *cl = nullptr): p_class(cl), p_funcs(nullptr), p_nfuncs(0) {}

    Event(const Event &ev): p_class(ev.p_class), p_nfuncs(ev.p_nfuncs) {
        using Func = Function<void(C &, A...)>;
        Func *nbuf = (Func *)new byte[sizeof(Func) * p_nfuncs];
        for (Size i = 0; i < p_nfuncs; ++i)
            new (&nbuf[i]) Func(ev.p_funcs[i]);
        p_funcs = nbuf;
    }

    Event(Event &&ev): p_class(ev.p_class), p_funcs(ev.p_funcs),
                       p_nfuncs(ev.p_nfuncs) {
        ev.p_class = nullptr;
        ev.p_funcs = nullptr;
        ev.p_nfuncs = 0;
    }

    ~Event() {
        for (Size i = 0; i < p_nfuncs; ++i)
            p_funcs[i].~Function<void(C &, A...)>();
        delete[] (byte *)p_funcs;
    }

    template<typename F>
    Size connect(F func) {
        using Func = Function<void(C &, A...)>;
        Func *nbuf = (Func *)new byte[sizeof(Func) * (p_nfuncs + 1)];
        for (Size i = 0; i < p_nfuncs; ++i) {
            new (&nbuf[i]) Func(move(p_funcs[i]));
            p_funcs[i].~Func();
        }
        new (&nbuf[p_nfuncs]) Func(func);
        delete[] (byte *)p_funcs;
        p_funcs = nbuf;
        return p_nfuncs++;
    }

    bool disconnect(Size idx) {
        if ((idx >= p_nfuncs) || !p_funcs[idx]) return false;
        p_funcs[idx] = nullptr;
        return true;
    }

    template<typename ...Args>
    void emit(Args &&...args) {
        if (!p_class) return;
        for (Size i = 0; i < p_nfuncs; ++i)
            if (p_funcs[i]) p_funcs[i](*p_class, args...);
    }

    template<typename ...Args>
    void operator()(Args &&...args) {
        emit(forward<Args>(args)...);
    }

private:
    C *p_class;
    Function<void(C &, A...)> *p_funcs;
    Size p_nfuncs;
};

} /* namespace ostd */

#endif