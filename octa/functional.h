/* Function objects for OctaSTD.
 *
 * This file is part of OctaSTD. See COPYING.md for futher information.
 */

#ifndef OCTA_FUNCTIONAL_H
#define OCTA_FUNCTIONAL_H

namespace octa {
#define OCTA_DEFINE_BINARY_OP(name, op, rettype) \
    template<typename T> struct name { \
        bool operator()(const T &x, const T &y) const { return x op y; } \
        struct type { \
            typedef T first; \
            typedef T second; \
            typedef rettype result; \
        }; \
    };

    OCTA_DEFINE_BINARY_OP(Less, <, bool)
    OCTA_DEFINE_BINARY_OP(LessEqual, <=, bool)
    OCTA_DEFINE_BINARY_OP(Greater, >, bool)
    OCTA_DEFINE_BINARY_OP(GreaterEqual, >=, bool)
    OCTA_DEFINE_BINARY_OP(Equal, ==, bool)
    OCTA_DEFINE_BINARY_OP(NotEqual, !=, bool)
    OCTA_DEFINE_BINARY_OP(LogicalAnd, &&, bool)
    OCTA_DEFINE_BINARY_OP(LogicalOr, ||, bool)
    OCTA_DEFINE_BINARY_OP(Modulus, %, T)
    OCTA_DEFINE_BINARY_OP(Multiplies, *, T)
    OCTA_DEFINE_BINARY_OP(Divides, /, T)
    OCTA_DEFINE_BINARY_OP(Plus, +, T)
    OCTA_DEFINE_BINARY_OP(Minus, -, T)
    OCTA_DEFINE_BINARY_OP(BitAnd, &, T)
    OCTA_DEFINE_BINARY_OP(BitOr, |, T)
    OCTA_DEFINE_BINARY_OP(BitXor, ^, T)

#undef OCTA_DEFINE_BINARY_OP

    template<typename T> struct LogicalNot {
        bool operator()(const T &x) const { return !x; }
        struct type {
            typedef T argument;
            typedef bool result;
        };
    };

    template<typename T> struct Negate {
        bool operator()(const T &x) const { return -x; }
        struct type {
            typedef T argument;
            typedef T result;
        };
    };

    template<typename T> struct BinaryNegate {
        struct type {
            typedef typename T::type::first first;
            typedef typename T::type::second second;
            typedef bool result;
        };
        explicit BinaryNegate(const T &f): p_fn(f) {}
        bool operator()(const typename type::first &x,
                        const typename type::second &y) {
            return !p_fn(x, y);
        }
    private:
        T p_fn;
    };

    template<typename T> struct UnaryNegate {
        struct type {
            typedef typename T::type::argument argument;
            typedef bool result;
        };
        explicit UnaryNegate(const T &f): p_fn(f) {}
        bool operator()(const typename type::argument &x) {
            return !p_fn(x);
        }
    private:
        T p_fn;
    };

    template<typename T> UnaryNegate<T> not1(const T &fn) {
        return UnaryNegate<T>(fn);
    }

    template<typename T> BinaryNegate<T> not2(const T &fn) {
        return BinaryNegate<T>(fn);
    }
}

#endif