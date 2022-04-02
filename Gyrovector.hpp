#pragma once

template<typename T>
struct Gyrovector {
    std::complex<T> val;

    inline T x() const { return val.real(); }
    inline T y() const { return val.imag(); }

    inline T norm() const { return std::norm(val); }
    inline T abs() const  { return std::abs(val); }
    inline bool isZero() const { return val.real() == 0.0 && val.imag() == 0.0; }

    Gyrovector(const T k) : val(std::complex(k, 0.0)) {}
    Gyrovector(const T x, const T y) : val(std::complex(x, y)) {}
    Gyrovector(const std::complex<T> z) : val(z) {}

    T operator,(const Gyrovector<T> & N) {
        return val.real() * N.val.real() + val.imag() * N.val.imag();
    }

    inline auto add(const Gyrovector<T> & N) const { return Gyrovector<T>(val + N.val); }
    inline auto conj() const { return Gyrovector<T>(std::conj(val)); }
    inline auto scale(const T k) const { return Gyrovector<T>(k * val); }
    inline auto mult(const Gyrovector<T> & N) const { return Gyrovector<T>(val * N.val); }
    inline auto inv() const { return Gyrovector<T>(1.0 / val); }
    inline auto div(const Gyrovector<T> & N) const { return Gyrovector<T>(val / N.val); }

    inline auto operator-() const { return Gyrovector<T>(-val); }
    inline auto operator+() const { return *this; }
};

template<typename T> auto operator+(const Gyrovector<T> & A, const Gyrovector<T> & B) {
    return A.add(B).div(Gyrovector<T>(1).add(A.conj().mult(B)));
}

template<typename T> Gyrovector<T> operator*(const T k, const Gyrovector<T> & A) {
    return A.isZero() ? A : A.scale(tanh(k * atanh(A.abs())) / A.abs());
}

template<typename T> Gyrovector<T> gyr(const Gyrovector<T> & A, const Gyrovector<T> & B, const Gyrovector<T> & C) {
    return -(A + B) + (A + (B + C));
}

template<typename T> std::function<Gyrovector<T>(Gyrovector<T>)> Gyr(const Gyrovector<T> & A, const Gyrovector<T> & B) {
    return [A, B](Gyrovector<T> C) { return gyr(A, B, C); };
}

template<typename T> Gyrovector<T> Coadd(const Gyrovector<T> & A, const Gyrovector<T> & B) {
    return A + gyr(A, -B, B);
}

template<typename T> std::function<Gyrovector<T>(T)> Line(const Gyrovector<T> & A, const Gyrovector<T> & B) {
    auto N = (-A) + B; return [A, N](T t) { return A + (t * N); };
}

template<typename T> Gyrovector<T> midpoint(const Gyrovector<T> & A, const Gyrovector<T> & B) {
    return A + 0.5 * (-A + B);
}

template<typename T> std::function<Gyrovector<T>(T)> Coline(const Gyrovector<T> & A, const Gyrovector<T> & B) {
    auto N = Coadd(B, -A); return [A, N](T t) { return (t * N) + A; };
}

template<typename T> std::function<Gyrovector<T>(Gyrovector<T>)> Translate(const Gyrovector<T> & N) {
    return [N](Gyrovector<T> A) { return N + A; };
}

template<typename T> std::function<Gyrovector<T>(Gyrovector<T>)> Scalar(const T k) {
    return [k](Gyrovector<T> A) { return k * A; };
}

template<typename T>
struct Möbius {
    std::complex<T> a, b, c, d;
    std::complex<T> det() const { return a * d - b * c; }

    inline Möbius<T> div(std::complex<T> k) const { return {a / k, b / k, c / k, d / k}; }
    inline Möbius<T> normalize() const { return div(det()); }

    Gyrovector<T> apply(Gyrovector<T> & w) const {
        return (a * w.val + b) / (c * w.val + d);
    }

    inline Möbius<T> inverse() const { return Möbius<T>(d, -b, -c, a).normalize(); }

    static inline Möbius<T> identity() { return {1, 0, 0, 1}; }

    static Möbius<T> translate(const Gyrovector<T> & N) {
        return {1, N.val, std::conj(N.val), 1};
    }
};

template<typename T> Möbius<T> operator*(const Möbius<T> & A, const Möbius<T> & B) {
    return {
        A.a * B.a + A.b * B.c,
        A.a * B.b + A.b * B.d,
        A.c * B.a + A.d * B.c,
        A.c * B.b + A.d * B.d
    };
}

template<typename T> std::function<Gyrovector<T>(Gyrovector<T>)> Transform(const Möbius<T> & M) {
    return [M](Gyrovector<T> A) { return M.apply(A); };
}
