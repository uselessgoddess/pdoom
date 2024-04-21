#include "types.h"

struct Vertex {
    f32 position[3];
};

struct Triangle {
    Vertex vertices[3];
    vec3f uv[3];
};

using Rgb = std::array<uint8_t, 3>;

struct Texture {
    uint8_t *ptr;
    size_t len;
    size_t width;
    size_t height;

    [[nodiscard]] auto get(size_t x, size_t y) const -> Rgb {
        auto idx = (y * width + x) * 3;
        return {
            uint8_t (ptr[idx + 0]),
            uint8_t (ptr[idx + 1]),
            uint8_t (ptr[idx + 2]),
        };
    }

    [[nodiscard]] auto uv(vec3f uv) const {
        uv->x = (float_t(1.0) - uv->x);
        uv->y = (float_t(1.0) - uv->y);
        return vec2i(uv->x * float_t(width), uv->y * float_t(height));
    }
};

struct ObjRepr {
    Triangle *triangles;
    size_t triangles_len;
    Texture texture;
};

extern "C" ObjRepr load_elemental();
