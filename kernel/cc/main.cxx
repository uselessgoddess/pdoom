#include "std/array"
#include "std/limits"
#include "std/span"
#include "matrix.h"
#include "rand.h"
#include "model.h"

using std::uint32_t;
using std::uint64_t;
using std::uint8_t;

constexpr size_t DESCALE_FACTOR = 4;
constexpr size_t WIDTH = 1280 / DESCALE_FACTOR;
constexpr size_t HEIGHT = 720 / DESCALE_FACTOR;
constexpr size_t DEPTH = 255;

extern "C" void *malloc(size_t);

template <typename T>
constexpr T abs(T t) {
  return (t < 0) ? -t : t;
}

template <size_t D>
using Color = std::array<uint8_t, D>;

template <size_t D>
struct FrameBuffer : std::span<uint8_t> {
  int32_t z_buffer[HEIGHT * WIDTH] = {0};

  FrameBuffer(std::span<uint8_t> span) : std::span<uint8_t>(span) {
    before_update();
  }

  void before_update() {
    for (auto &it : z_buffer) {
      it = std::numeric_limits<int>::min();
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

  void triangle(vec3i t0, vec3i t1, vec3i t2, vec2i uv0, vec2i uv1, vec2i uv2, f32 intensity,
                Texture texture) {
    if (t0->y == t1->y && t0->y == t2->y)
      return;

    if (t0->y > t1->y) {
      std::swap(t0, t1);
      std::swap(uv0, uv1);
    }
    if (t0->y > t2->y) {
      std::swap(t0, t2);
      std::swap(uv0, uv2);
    }
    if (t1->y > t2->y) {
      std::swap(t1, t2);
      std::swap(uv1, uv2);
    }

    int total_height = t2->y - t0->y;
    for (int i = 0; i < total_height; i++) {
      bool second_half = i > t1->y - t0->y || t1->y == t0->y;
      int segment_height = (second_half) ? t2->y - t1->y : t1->y - t0->y;
      float alpha = (float)i / total_height;
      float beta = (float)(i - (second_half ? t1->y - t0->y : 0)) /
                   segment_height;  // be careful: with above conditions no division by zero here
      vec3i A = t0 + vec3i(vec3f(t2 - t0) * alpha);
      vec3i B = second_half ? t1 + vec3f(t2 - t1) * beta : t0 + vec3f(t1 - t0) * beta;
      vec2i uvA = uv0 + (uv2 - uv0) * alpha;
      vec2i uvB = second_half ? uv1 + (uv2 - uv1) * beta : uv0 + (uv1 - uv0) * beta;
      if (A->x > B->x) {
        std::swap(A, B);
        std::swap(uvA, uvB);
      }
      for (int j = A->x; j <= B->x; j++) {
        float phi = (B->x == A->x) ? 1.0 : float(j - A->x) / float(B->x - A->x);
        vec3i P = vec3f(A) + vec3f(B - A) * phi;
        P->x = j;
        P->y = t0->y + i;
        vec2i uvP = uvA + (uvB - uvA) * phi;
        int idx = P->x + P->y * WIDTH;
        if (z_buffer[idx] < P->z) {
          z_buffer[idx] = P->z;
          auto color = texture.get(uvP->x, uvP->y);
          this->set(P->x, P->y,
                    {
                        uint8_t(color[0] * intensity),
                        uint8_t(color[1] * intensity),
                        uint8_t(color[2] * intensity),
                    });
        }
      }
    }
  }
};

constexpr size_t PLACE_LEN = WIDTH * HEIGHT * 3;

extern "C" void kernel_main(uint8_t *buf, uint32_t len) {
  auto place = (uint8_t *)malloc(PLACE_LEN);
  auto frame = FrameBuffer<3>({place, PLACE_LEN});

  auto model = load_elemental();

  auto light_dir = vec3f{0, 0, -1};
  for (int i = 0; i < PLACE_LEN; i++) {
    place[i] = 255;
  }

  while (true) {
    frame.before_update();

    for (size_t i = 0; i < model.triangles_len; i++) {
      auto triangle = model.triangles[i];

      vec3i screen_coords[3];
      vec3f world_coords[3];
      for (int j = 0; j < 3; j++) {
        auto world = triangle.vertices[j].position;
        screen_coords[j] =
            vec3i{int32_t((world[0] + 1.0) * WIDTH / 2.0), int32_t((world[1] + 1.0) * HEIGHT / 2.0),
                  int32_t((world[2] + 1.0) * DEPTH / 2.0)};
        world_coords[j] = {world[0], world[1], world[2]};
      }

      vec3f n = (world_coords[2] - world_coords[0]).cross(world_coords[1] - world_coords[0]);
      float intensity = light_dir.dot(n.normalized());
      if (intensity > 0) {
        vec2i uv[3];
        for (int k = 0; k < 3; k++) {
          uv[k] = model.texture.uv(triangle.uv[k]);
        }
        frame.triangle(screen_coords[0], screen_coords[1], screen_coords[2], uv[0], uv[1], uv[2],
                       intensity, model.texture);
      }
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
