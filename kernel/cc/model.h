#include "types.h"

struct Vertex {
  vec3f position;
};

struct Triangle {
  Vertex vertices[3];
  vec3f uv[3];
};

using Rgb = std::array<uint8_t, 3>;

constexpr float_t clamp1(float_t t) {
  if (t >= 0.0 || t <= 1.0) {
    return 1.0 - t;
  } else {
    return 0.0;
  }
}

struct Texture {
  uint8_t *ptr;
  size_t len;
  size_t width;
  size_t height;

  [[nodiscard]] auto get(size_t x, size_t y) const -> Rgb {
    auto idx = (y * width + x) * 3;
    return {
        uint8_t(ptr[idx + 0]),
        uint8_t(ptr[idx + 1]),
        uint8_t(ptr[idx + 2]),
    };
  }

  [[nodiscard]] auto diffuse(vec2f uv) const {
    auto ui = vec2i(clamp1(uv->x) * float_t(width), clamp1(uv->y) * float_t(height));
    return get(ui->x, ui->y);
  }
};

struct ObjRepr {
  Triangle *triangles;
  size_t triangles_len;
  Texture texture;
};

extern "C" ObjRepr load_elemental();
