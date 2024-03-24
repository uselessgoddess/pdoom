#include "std/concepts"

constexpr void assert(bool b) {}

template<typename T, size_t N>
struct vec;

template<typename T, size_t N>
vec<T, N> operator*(vec<T, N> lhs, std::convertible_to<T> auto rhs) {
    for (int i = N; i--; lhs[i] *= rhs);
    return lhs;
}

template<typename T, size_t N>
T operator*(vec<T, N> lhs, vec<T, N> rhs) {
    T ret = 0;
    for (int i = N; i--; ret += lhs[i] * rhs[i]);
    return ret;
}

template<typename T, size_t N>
vec<T, N> operator+(vec<T, N> lhs, vec<T, N> rhs) {
    for (int i = N; i--; lhs[i] += rhs[i]);
    return lhs;
}

template<typename T, size_t N>
vec<T, N> operator-(vec<T, N> lhs, vec<T, N> rhs) {
    for (int i = N; i--; lhs[i] -= rhs[i]);
    return lhs;
}


template<typename T>
struct vec<T, 2> {
    T x, y;

    constexpr T &operator[](size_t idx) {
        return idx ? y : x;
    }

    constexpr T operator[](size_t idx) const {
        return idx ? y : x;
    }
};

using vec2i = vec<int32_t, 2>;
