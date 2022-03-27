#pragma once

template<class T>
struct Gyrovector {
    std::complex<T> value;

    inline double x() const { return value.real(); }
    inline double y() const { return value.imag(); }

    inline double norm() const { return std::norm(value); }
    inline double abs() const  { return std::abs(value); }
    inline bool isZero() const { return value.real() == 0.0 && value.imag() == 0.0; }

    Gyrovector(const double k) : value(std::complex(k, 0.0)) {}
    Gyrovector(const double x, const double y) : value(std::complex(x, y)) {}
    Gyrovector(const std::complex<double> z) : value(z) {}

    T operator,(const Gyrovector<T> & N) {
        return value.real() * N.value.real() + value.imag() * N.value.imag();
    }

    inline auto add(const Gyrovector<T> & N) const { return Gyrovector<T>(value + N.value); }
    inline auto conj() const { return Gyrovector<T>(std::conj(value)); }
    inline auto scale(const T k) const { return Gyrovector<T>(k * value); }
    inline auto mult(const Gyrovector<T> & N) const { return Gyrovector<T>(value * N.value); }
    inline auto inv() const { return Gyrovector<T>(1.0 / value); }
    inline auto div(const Gyrovector<T> & N) const { return Gyrovector<T>(value / N.value); }

    inline auto operator-() const { return Gyrovector<T>(-value); }
    inline auto operator+() const { return *this; }
};

template<class T> auto operator+(const Gyrovector<T> & A, const Gyrovector<T> & B) {
    return A.add(B).div(Gyrovector<T>(1).add(A.conj().mult(B)));
}

template<class T> Gyrovector<T> operator*(const T k, const Gyrovector<T> & A) {
    return A.isZero() ? A : A.scale(tanh(k * atanh(A.abs())) / A.abs());
}

template<class T> Gyrovector<T> gyr(const Gyrovector<T> & A, const Gyrovector<T> & B, const Gyrovector<T> & C) {
    return -(A + B) + (A + (B + C));
}

template<class T> std::function<Gyrovector<T>(Gyrovector<T>)> Gyr(const Gyrovector<T> & A, const Gyrovector<T> & B) {
    return [A, B](Gyrovector<T> C) { return gyr(A, B, C); };
}

template<class T> Gyrovector<T> Coadd(const Gyrovector<T> & A, const Gyrovector<T> & B) {
    return A + gyr(A, -B, B);
}

template<class T> std::function<Gyrovector<T>(T)> Line(const Gyrovector<T> & A, const Gyrovector<T> & B) {
    auto N = (-A) + B; return [A, N](double t) { return A + (t * N); };
}

template<class T> Gyrovector<T> midpoint(const Gyrovector<T> & A, const Gyrovector<T> & B) {
    return A + 0.5 * (-A + B);
}

template<class T> std::function<Gyrovector<T>(T)> Coline(const Gyrovector<T> & A, const Gyrovector<T> & B) {
    auto N = Coadd(B, -A); return [A, N](T t) { return (t * N) + A; };
}

template<class T> std::function<Gyrovector<T>(Gyrovector<T>)> Translate(const Gyrovector<T> & N) {
    return [N](Gyrovector<T> A) { return N + A; };
}

template<class T> std::function<Gyrovector<T>(Gyrovector<T>)> Scalar(const T k) {
    return [k](Gyrovector<T> A) { return k * A; };
}

template<class T>
struct Möbius {
    std::complex<T> a, b, c, d;
    std::complex<T> det() const { return a * d - b * c; }

    inline Möbius<T> div(std::complex<T> k) const { return {a / k, b / k, c / k, d / k}; }
    inline Möbius<T> normalize() const { return div(det()); }

    Gyrovector<T> apply(Gyrovector<T> & w) const {
        return (a * w.value + b) / (c * w.value + d);
    }

    inline Möbius<T> inverse() const { return Möbius<T>(d, -b, -c, a).normalize(); }

    static inline Möbius<T> identity() { return {1, 0, 0, 1}; }

    static Möbius<T> translate(const Gyrovector<T> & N) {
        return {1, N.value, std::conj(N.value), 1};
    }
};

template<class T> Möbius<T> operator*(const Möbius<T> & A, const Möbius<T> & B) {
    return {
        A.a * B.a + A.b * B.c,
        A.a * B.b + A.b * B.d,
        A.c * B.a + A.d * B.c,
        A.c * B.b + A.d * B.d
    };
}

template<class T> std::function<Gyrovector<T>(Gyrovector<T>)> Transform(const Möbius<T> & M) {
    return [M](Gyrovector<T> A) { return M.apply(A); };
}
