#include "std/array"
#include "std/limits"
#include "std/span"
#include "matrix.h"
#include "rand.h"
#include "model.h"
#include "gl.hxx"

template <size_t M>
auto embed(const auto &v, float_t fill = 1) {
  vector<float_t, M> ret;
  for (size_t i = M; i--;) {
    ret[i] = (i < v.shape()[0]) ? v[i] : fill;
  }
  return ret;
}

using std::uint32_t;
using std::uint64_t;
using std::uint8_t;

constexpr size_t DESCALE_FACTOR = 1;
constexpr size_t WIDTH = 1280 / DESCALE_FACTOR;
constexpr size_t HEIGHT = 720 / DESCALE_FACTOR;
constexpr size_t DEPTH = 255;

extern "C" void *malloc(size_t);

template <typename T>
constexpr T abs(T t) {
  return (t < 0) ? -t : t;
}

void barycentric(vec2f tr[3], vec2f P, vec3f &in_place) {
  auto ABC = mat3x3({embed<3>(tr[0]), embed<3>(tr[1]), embed<3>(tr[2])});
  if (ABC.det() < 1e-3) {
    in_place = vec3f{-1, 1, 1};
  } else {
    in_place = ABC.invert_transpose() * embed<3>(P);
  }
}

template <size_t D>
struct FrameBuffer : std::span<uint8_t> {
  float_t z_buffer[HEIGHT * WIDTH] = {0};

  mat4x4 camera;
  mat4x4 viewport;
  mat4x4 projection;

  explicit FrameBuffer(std::span<uint8_t> span) : std::span<uint8_t>(span) {}

  void before_update(/* viewport   */ size_t x, size_t y, size_t w, size_t h,
                     /* lookat     */ vec3f eye, vec3f center, vec3f up,
                     /* projection */ float_t coeff) {
    {
      // clang-format off
      viewport = {
        float_t(w) / 2, 0, 0, x + float_t(w) / 2,
        0, float_t(h) / 2, 0, y + float_t(h) / 2,
        0, 0, 1, 0,
        0, 0, 0, 1,
      };

      projection = {
        1,  0, 0, 0,
        0, -1, 0, 0,
        0,  0, 1, 0,
        0,  0,-1.0 / coeff,0
      };

      vec3f z = (eye - center).normalized();
      vec3f x = up.cross(z).normalized();
      vec3f y = z.cross(x).normalized();
      mat4x4 minv = {
        x->x,x->y,x->z,0,
        y->x,y->y,y->z,0,
        z->x,z->y,z->z,0,
        0,0,0,1
      };
      mat4x4 tr   = {
        1,0,0,-eye->x,
        0,1,0,-eye->y,
        0,0,1,-eye->z,
        0,0,0,1
      };
      camera = minv * tr;
      // clang-format on
    }

    for (auto &it : z_buffer) {
      it = std::numeric_limits<float_t>::max();
    }
  }

  void set(size_t x, size_t y, Color<D> color) {
    if (x > WIDTH || y > HEIGHT) {
      return;
    }
    auto start = (y * WIDTH + x) * D;
    if (start > this->size() || start + D > this->size()) {
      return;
    }
    for (size_t i = 0; i < D; i++) {
      (*this)[start + i] = color[i];
    }
  }

  struct CoreShader {
    mat2x3 uv = {};

    auto vertex(const FrameBuffer<3> &frame, Triangle triangle, size_t nvert) -> vec4f {
      uv.set_col(nvert, embed<2>(triangle.uv[nvert]));
      return frame.projection * (frame.camera * embed<4>(triangle.vertices[nvert].position));
    }

    auto fragment(Texture texture, vec3f bar, Color<3> &color) -> bool {
      color = texture.diffuse(uv * bar);
      return false;
    }
  };

  void triangle(vec4f verts[4], CoreShader shader, Texture texture) {
    vec4f pts[3] = {viewport * verts[0], viewport * verts[1], viewport * verts[2]};
    vec2f pts2[3] = {};
    for (size_t i = 0; i < 3; i++) {
      pts2[i] = embed<2>(pts[i] / pts[i][3]);
    }

    int32_t b_boxmin[2] = {WIDTH - 1, HEIGHT - 1};
    int32_t b_boxmax[2] = {0, 0};

    for (auto &pt : pts2) {
      for (size_t j = 0; j < 2; j++) {
        b_boxmin[j] = std::min(b_boxmin[j], static_cast<int32_t>(pt[j]));
        b_boxmax[j] = std::max(b_boxmax[j], static_cast<int32_t>(pt[j]));
      }
    }

    Color<3> color = {M_Random(), M_Random(), M_Random()};
    for (int32_t x = std::max(b_boxmin[0], 0); x <= std::min(b_boxmax[0], int32_t(WIDTH - 1));
         x++) {
      for (int32_t y = std::max(b_boxmin[1], 0); y <= std::min(b_boxmax[1], int32_t(HEIGHT - 1));
           y++) {
        vec3f bc_screen = {};
        barycentric(pts2, {static_cast<float_t>(x), static_cast<float_t>(y)}, bc_screen);
        vec3f bc_clip = {bc_screen->x / pts[0][3], bc_screen->y / pts[1][3],
                         bc_screen->z / pts[2][3]};
        bc_clip = bc_clip / (bc_clip->x + bc_clip->y + bc_clip->z);
        float_t frag_depth = vec3f{verts[0][2], verts[1][2], verts[2][2]}.dot(bc_clip);
        if (bc_screen->x < 0 || bc_screen->y < 0 || bc_screen->z < 0
            // || frag_depth > z_buffer[x + y * WIDTH]
            ) {
          continue;
        }
        Color<3> color = {M_Random(), M_Random(), M_Random()};
        if (!shader.fragment(texture, bc_clip, color)) {
          z_buffer[x + y * WIDTH] = frag_depth;
          this->set(x, y, color);
        }
      }
    }
  }
};

constexpr size_t PLACE_LEN = WIDTH * HEIGHT * 3;

extern "C" void kernel_main(uint8_t *buf, uint32_t len) {
  vec3f light_dir{1, 1, 1};  // light source
  vec3f eye{0, -1, 0};        // camera position
  vec3f center{0, 0, 0};     // camera direction
  vec3f up{0, 1, 0};         // camera up vector

  auto place = (uint8_t *)malloc(PLACE_LEN);
  auto frame = FrameBuffer<3>({place, PLACE_LEN});

  auto model = load_elemental();
  while (true) {
    frame.before_update( /* viewport */ WIDTH / 8, HEIGHT / 8, WIDTH * 3 / 4, HEIGHT * 3 / 4,
                        /* camera   */ eye, center, up,
                        /* projection */ 1.0 / (eye - center).norm());

    for (int i = 0; i < PLACE_LEN; i++) {
      place[i] = 255;
    }

    // eye->x -= 0.1;
    eye->z -= 0.011;
    eye->y += 0.11;

    for (size_t i = 0; i < model.triangles_len; i++) {
      auto triangle = model.triangles[i];

      FrameBuffer<3>::CoreShader shader = {};
      vec4f verts[3];
      for (int k = 0; k < 3; k++) {
        verts[k] = shader.vertex(frame, triangle, k);
      }
      frame.triangle(verts, shader, model.texture);
    }

    for (size_t i = 0; i < WIDTH; i++) {
      for (size_t j = 0; j < HEIGHT; j++) {
        for (size_t k1 = 0; k1 < DESCALE_FACTOR; k1++) {
          for (size_t k2 = 0; k2 < DESCALE_FACTOR; k2++) {
            for (size_t c = 0; c < 3; c++) {
              buf[((j * DESCALE_FACTOR + k1) * WIDTH * DESCALE_FACTOR + (i * DESCALE_FACTOR + k2)) *
                      3 +
                  c] = place[(j * WIDTH + i) * 3 + c];
            }
          }
        }
      }
    }
  }
}
