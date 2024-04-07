#include "std/concepts"
#include "types.h"

constexpr void assert(bool b) {}

template<typename T, char iterations = 2>
constexpr T inv_sqrt(T x) {
    using Repr = std::conditional_t<sizeof(T) == 8, std::int64_t, std::int32_t>;

    T y = x;
    T x2 = y * 0.5;

    Repr i = *reinterpret_cast<Repr *>(&y);

    i = (sizeof(T) == 8 ? 0x5fe6eb50c7b537a9 : 0x5f3759df) -
        (i >> 1);
    y = *reinterpret_cast<T *>(&i);
    y = y * (1.5 - (x2 * y * y));
    if (iterations == 2) {
        y = y * (1.5 - (x2 * y * y));
    }
    return y;
}

template<typename T>
constexpr T sqrt(T x) {
    return 1.0 / inv_sqrt(x);
}

template<typename T, size_t N>
struct vec;

template<typename T, size_t N>
vec<T, N> operator*(vec<T, N> lhs, std::convertible_to<T> auto rhs) {
    for (int i = N; i--; lhs[i] *= rhs);
    return lhs;
}

template<typename T, size_t N>
vec<T, N> operator/(vec<T, N> lhs, std::convertible_to<T> auto rhs) {
    for (int i = N; i--; lhs[i] /= rhs);
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

template<typename T>
struct vec<T, 3> {
    T x, y, z;

    constexpr T &operator[](size_t idx) {
        return idx ? (1 == idx ? y : z) : x;
    }

    constexpr T operator[](size_t idx) const {
        return idx ? (1 == idx ? y : z) : x;
    }

    constexpr auto operator^(vec<T, 3> v) const {
        return vec(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
    }

    T norm_squared() const {
        return *this * *this;
    }

    T norm() const {
        return sqrt(norm_squared());
    }

    auto normalize() const {
        return *this / norm();
    }
};

using vec2i = vec<int32_t, 2>;
using vec2f = vec<f32, 2>;
using vec3f = vec<f32, 3>;
