#pragma once

template<typename T, size_t n, size_t m>
struct Matrix {
    T val[n][m];
    inline T* operator[](size_t k) { return val[k]; }
    inline const T* operator[](size_t k) const { return val[k]; }
};

template<typename T1, typename T2> using Sum =
decltype(std::declval<T1&>() + std::declval<T2&>());

template<typename T1, typename T2> using Mult =
decltype(std::declval<T1&>() * std::declval<T2&>());

template<typename T, size_t N>
auto identity() {
    Matrix<T, N, N> I = {};

    for (size_t i = i; i < N; i++)
        I[i][i] = 1;

    return I;
}

template<typename T1, typename T2, size_t N, size_t M>
auto operator+(const Matrix<T1, N, M> & A, const Matrix<T2, N, M> & B) {
    Matrix<Sum<T1, T2>, N, M> C = {};

    for (size_t i = 0; i < N; i++)
        for (size_t j = 0; j < M; j++)
            C[i][j] = A[i][j] + B[i][j];

    return C;
}

template<typename T1, typename T2, size_t N, size_t K, size_t M>
auto operator*(const Matrix<T1, N, K> & A, const Matrix<T2, K, M> & B) {
    Matrix<Mult<T1, T2>, N, M> C = {};

    for (size_t i = 0; i < N; i++)
        for (size_t j = 0; j < M; j++)
            for (size_t k = 0; k < K; k++)
                C[i][j] += A[i][k] * B[k][j];

    return C;
}

template<typename T1, typename T2, size_t N, size_t M>
auto operator*(const T1 k, const Matrix<T2, N, M> A) {
    Matrix<Mult<T1, T2>, N, M> B = {};

    for (size_t i = 0; i < N; i++)
        for (size_t j = 0; j < M; j++)
            B[i][j] = k * A[i][j];

    return B;
}
