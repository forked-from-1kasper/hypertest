#pragma once

#include <concepts>
#include <cstdint>
#include <cmath>

#include <gmpxx.h>

constexpr size_t Byte  = sizeof(uint8_t);
constexpr size_t Word  = sizeof(uint16_t);
constexpr size_t Dword = sizeof(uint32_t);
constexpr size_t Qword = sizeof(uint64_t);

namespace Math {
    template<typename T> extern const T zero;
    template<typename T> extern const T one;

    template<typename T> void half(T &)         = delete;
    template<typename T> void twice(T &)        = delete;
    template<typename T> bool odd(const T &)    = delete;
    template<typename T> bool isZero(const T &) = delete;
    template<typename T> bool isUnit(const T &) = delete;
    template<typename T> bool isNeg(const T &)  = delete;

    template<typename T> bool equal(const T &, const T &) = delete;
    template<typename T> inline bool differ(const T & t₁, const T & t₂) { return !equal<T>(t₁, t₂); }

    template<typename T> void divexact(T &, const T &, const T &) = delete;

    template<typename T, typename U> U field(const T &) = delete;

    template<typename T> void * serialize(const T &, size_t &) = delete;
}

template<typename T> concept EuclideanDomain =
requires(T a, T b, T c, size_t k) {
    {            -a             } -> std::convertible_to<T>;
    {           a + b           } -> std::convertible_to<T>;
    {           a - b           } -> std::convertible_to<T>;
    {           a * b           } -> std::convertible_to<T>;
    {           a / b           } -> std::convertible_to<T>;
    { Math::zero<T>             } -> std::convertible_to<T>;
    { Math::one<T>              } -> std::convertible_to<T>;
    { Math::divexact(a, b, c)   } -> std::same_as<void>;
    { Math::half(a)             } -> std::same_as<void>;
    { Math::twice(a)            } -> std::same_as<void>;
    { Math::odd(a)              } -> std::same_as<bool>;
    { Math::isZero(a)           } -> std::same_as<bool>;
    { Math::isUnit(a)           } -> std::same_as<bool>;
    { Math::isNeg(a)            } -> std::same_as<bool>;
    { Math::equal(a, b)         } -> std::same_as<bool>;
    { Math::field<T, float>(a)  } -> std::same_as<float>;
    { Math::field<T, double>(a) } -> std::same_as<double>;
    { Math::serialize(a, k)     } -> std::same_as<void*>;
};

namespace Math {
    template<> inline int64_t zero<int64_t> = 0;
    template<> inline int64_t one<int64_t> = 1;

    template<> inline void divexact<int64_t>(int64_t & q, const int64_t & n, const int64_t & d) { q = n / d; };

    template<> inline void twice<int64_t>(int64_t & n) { n <<= 1; }
    template<> inline void half<int64_t>(int64_t & n) { n >>= 1; }

    template<> inline bool odd<int64_t>(const int64_t & n) { return bool(n % 2); }

    template<> inline bool isZero<int64_t>(const int64_t & n) { return n == 0; }
    template<> inline bool isUnit<int64_t>(const int64_t & n) { return std::abs(n) == 1; }

    template<> inline bool isNeg<int64_t>(const int64_t & n) { return n < 0; }

    template<> inline bool equal<int64_t>(const int64_t & n, const int64_t & m) { return n == m; }

    template<> inline float field<int64_t, float>(const int64_t & n) { return float(n); }
    template<> inline double field<int64_t, double>(const int64_t & n) { return double(n); }

    template<> inline void * serialize<int64_t>(const int64_t & n, size_t & k)
    { auto retval = new int64_t; *retval = std::abs(n); k = sizeof(int64_t); return retval; }
}

namespace Math {
    template<> inline mpz_class zero<mpz_class> = mpz_class(0);
    template<> inline mpz_class one<mpz_class> = mpz_class(1);

    template<> inline void divexact<mpz_class>(mpz_class & q, const mpz_class & n, const mpz_class & d)
    { mpz_divexact(q.get_mpz_t(), n.get_mpz_t(), d.get_mpz_t()); };

    template<> inline void twice<mpz_class>(mpz_class & n) { mpz_mul_2exp(n.get_mpz_t(), n.get_mpz_t(), 1); }
    template<> inline void half<mpz_class>(mpz_class & n) { mpz_fdiv_q_2exp(n.get_mpz_t(), n.get_mpz_t(), 1); }

    template<> inline bool odd<mpz_class>(const mpz_class & n) { return bool(mpz_odd_p(n.get_mpz_t())); }

    template<> inline bool isZero<mpz_class>(const mpz_class & n) { return mpz_size(n.get_mpz_t()) == 0; }
    template<> inline bool isUnit<mpz_class>(const mpz_class & n) { return mpz_cmpabs(n.get_mpz_t(), one<mpz_class>.get_mpz_t()) == 0; }

    template<> inline bool isNeg<mpz_class>(const mpz_class & n) { return mpz_sgn(n.get_mpz_t()) < 0; }

    template<> inline bool equal<mpz_class>(const mpz_class & n, const mpz_class & m) { return mpz_cmp(n.get_mpz_t(), m.get_mpz_t()) == 0; }

    template<> inline float field<mpz_class, float>(const mpz_class & n) { return float(mpz_get_d(n.get_mpz_t())); }
    template<> inline double field<mpz_class, double>(const mpz_class & n) { return mpz_get_d(n.get_mpz_t()); }

    template<> inline void * serialize<mpz_class>(const mpz_class & n, size_t & k)
    { return mpz_export(nullptr, &k, 1, 1, 0, 0, n.get_mpz_t()); }
}