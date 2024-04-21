#include "types.h"

template <typename T, char iterations = 2>
constexpr T inv_sqrt(T x) {
  using Repr = std::conditional_t<sizeof(T) == 8, std::int64_t, std::int32_t>;

  T y = x;
  T x2 = y * 0.5;

  Repr i = *reinterpret_cast<Repr *>(&y);

  i = (sizeof(T) == 8 ? 0x5fe6eb50c7b537a9 : 0x5f3759df) - (i >> 1);
  y = *reinterpret_cast<T *>(&i);
  y = y * (1.5 - (x2 * y * y));
  if (iterations == 2) {
    y = y * (1.5 - (x2 * y * y));
  }
  return y;
}

// clang-format off
template<typename T>
constexpr auto gemm(
  size_t M, size_t K, size_t N,
  bool accum,
  const T *ap, std::array<size_t, 2> a_strides,
  const T *bp, std::array<size_t, 2> b_strides,
  T *cp, std::array<size_t, 2> c_strides
) {
  for (size_t i_m = 0; i_m < M; i_m++) {
    for (size_t i_k = 0; i_k < K; i_k++) {
      for (size_t i_n = 0; i_n < N; i_n++) {
        auto a = ap[a_strides[0] * i_m + a_strides[1] * i_k];
        auto b = bp[b_strides[0] * i_k + b_strides[1] * i_n];
        auto &c = cp[c_strides[0] * i_m + c_strides[1] * i_n];
        // std::cout << (a * b) << "\n";
        if (accum) {
            c += a * b;
        } else {
            c = a * b;
        }
      }
    }
  }
}
// clang-format on

template <typename T>
constexpr T sqrt(T x) {
  if constexpr (std::same_as<T, f16>) {
    return 1.0 / inv_sqrt(float(x));
  } else {
    return 1.0 / inv_sqrt(x);
  }
}

template <typename T>
struct dim_2t {
  T x;
  T y;
};

template <typename T>
struct dim_3t {
  T x;
  T y;
  T z;
};

template <typename T>
constexpr bool never() {
  return false;
}

template <typename T, size_t D>
auto infer_dim() {
  if constexpr (D == 2) {
    return dim_2t<T>{};
  } else if constexpr (D == 3) {
    return dim_3t<T>{};
  } else {
    static_assert(never<T>());
  }
}

template <typename T, size_t R, size_t C>
using storage = std::array<std::array<T, R>, C>;

template <typename T, size_t R, size_t C>
struct matrix;

template <typename T, size_t R, size_t C>
struct matrix {
  storage<T, R, C> repr;

  constexpr matrix() = default;

  template <typename U>
  constexpr matrix(matrix<U, R, C> mat) {
    for (size_t i = 0; i < R * C; i++) {
      repr_iter()[i] = T(mat.repr_iter()[i]);
    }
  }

  constexpr matrix(auto... args)
    requires(sizeof...(args) == R * C)
  {
    auto in_place = std::array<T, R * C>{T(args)...};
    auto trans = *(reinterpret_cast<storage<T, C, R> *>(&in_place));
    for (size_t i = 0; i < R; i++) {
      for (size_t j = 0; j < C; j++) {
        repr[j][i] = trans[i][j];
      }
    }
  }

  auto operator[](size_t row, size_t col) -> T & {
    return repr[col][row];
  }

  auto operator[](size_t row, size_t col) const -> const T & {
    return repr[col][row];
  }

  auto operator[](size_t idx) -> T & {
    return repr_ptr()[idx];
  }

  auto operator[](size_t idx) const -> const T & {
    return repr_ptr()[idx];
  }

  [[nodiscard]] constexpr auto strides() const -> std::array<size_t, 2> {
    return {1, R};
  }

  constexpr auto *operator->()
    requires(C == 1)
  {
    return reinterpret_cast<decltype(infer_dim<T, R>()) *>(repr.begin());
  }

  constexpr const auto *operator->() const
    requires(C == 1)
  {
    return reinterpret_cast<const decltype(infer_dim<T, R>()) *>(repr.begin());
  }

  [[nodiscard]] constexpr auto repr_ptr() -> T * {
    return (T *)repr.begin();
  }
  [[nodiscard]] constexpr auto repr_ptr() const -> const T * {
    return (const T *)repr.begin();
  }

  [[nodiscard]] constexpr auto &&repr_iter() {
    return reinterpret_cast<std::array<T, R * C> &>(repr);
  }
  [[nodiscard]] constexpr auto &&repr_iter() const {
    return reinterpret_cast<const std::array<T, R * C> &>(repr);
  }

  [[nodiscard]] constexpr auto cross(const matrix &v2) const
    requires(R == 3 && C == 1)  // allow only vec3
  {
    const auto &v1 = *this;
    return matrix{v1->y * v2->z - v1->z * v2->y, v1->z * v2->x - v1->x * v2->z,
                  v1->x * v2->y - v1->y * v2->x};
  }

  [[nodiscard]] constexpr auto dot(const matrix &mat) const -> T {
    auto res = T{};

    for (size_t i = 0; i < C; i++) {
      for (size_t j = 0; j < R; j++) {
        res += (*this)[j, i] * mat[j, i];
      }
    }

    return res;
  }

  [[nodiscard]] constexpr auto norm_squared() const -> T {
    return this->dot(*this);
  }

  [[nodiscard]] constexpr auto norm() const -> T
    requires(std::floating_point<T>)
  {
    return sqrt(norm_squared());
  }

  [[nodiscard]]

  constexpr auto
  normalized() const
    requires(std::floating_point<T>)
  {
    return *this / norm();
  }

  auto operator*(T scalar) const -> matrix {
    auto self = *this;
    for (size_t i = 0; i < R * C; i++) {
      self.repr_ptr()[i] *= scalar;
    }
    return self;
  }

  template <size_t K>
  auto operator*(const matrix<T, C, K> &mat) const -> matrix<T, R, K> {
    auto place = matrix<T, R, K>{};

    gemm(R, C, K, true, this->repr_ptr(), this->strides(), mat.repr_ptr(), mat.strides(),
         place.repr_ptr(), place.strides());
    return place;
  }

  auto operator/(T scalar) const -> matrix {
    auto self = *this;
    for (auto &it : self.repr_iter()) {
      it /= scalar;
    }
    return self;
  }

  auto operator+(matrix mat) const -> matrix {
    auto self = *this;
    for (size_t i = 0; i < R * C; i++) {
      self.repr_ptr()[i] += mat.repr_ptr()[i];
    }
    return self;
  }

  auto operator-(matrix mat) const -> matrix {
    auto self = *this;
    for (size_t i = 0; i < R * C; i++) {
      self.repr_ptr()[i] -= mat.repr_ptr()[i];
    }
    return self;
  }
};

template <typename T, size_t D>
using vector = matrix<T, D, 1>;

template <typename T>
using vec2 = vector<T, 2>;

template <typename T>
using vec3 = vector<T, 3>;

using vec2f = vec2<float_t>;
using vec3f = vec3<float_t>;
using vec2i = vec2<int32_t>;
using vec3i = vec3<int32_t>;
