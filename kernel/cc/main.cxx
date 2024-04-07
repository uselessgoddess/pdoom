#include "std/cstdint"
#include "std/array"
#include "std/span"
#include "std/span"
#include "vec.h"
#include "rand.h"
#include "model.h"

using std::uint64_t;
using std::uint32_t;
using std::uint8_t;

extern "C" uint64_t cpu_time_us();

constexpr size_t WIDTH = 1280;
constexpr size_t HEIGHT = 720;

extern "C" void *malloc(size_t);

template<typename T>
constexpr T abs(T t) {
    return (t < 0) ? -t : t;
}

template<size_t D>
using Color = std::array<uint8_t, D>;

template<size_t D>
struct FrameBuffer : std::span<uint8_t> {
    FrameBuffer(std::span<uint8_t> span) : std::span<uint8_t>(span) {}

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

    void triangle(vec2i t0, vec2i t1, vec2i t2, Color<D> color) {
        if (t0.y == t1.y && t0.y == t2.y) return;

        if (t0.y > t1.y) std::swap(t0, t1);
        if (t0.y > t2.y) std::swap(t0, t2);
        if (t1.y > t2.y) std::swap(t1, t2);

        int total_height = t2.y - t0.y;
        for (int i = 0; i < total_height; i++) {
            bool second_half = i > t1.y - t0.y || t1.y == t0.y;
            int segment_height = second_half ? t2.y - t1.y : t1.y - t0.y;
            float alpha = (float) i / total_height;
            float beta = (float) (i - (second_half ? t1.y - t0.y : 0)) /
                         segment_height;
            vec2i A = t0 + (t2 - t0) * alpha;
            vec2i B = second_half ? t1 + (t2 - t1) * beta : t0 + (t1 - t0) * beta;
            if (A.x > B.x) std::swap(A, B);
            for (int j = A.x; j <= B.x; j++) {
                this->set(j, t0.y + i, color);
            }
        }
    }
};

extern "C" void kernel_main(uint8_t *buf, uint32_t len) {
    auto place = (uint8_t *) malloc(len);
    auto frame = FrameBuffer<3>({place, len});

    int y1 = 0;
    int y2 = 0;
    int x = 1200;

    float y = 1.0 / 0;

    auto model = load_elemental();

    auto light_dir = vec3f{0, 0, -1};
    while (true) {

        for (size_t i = 0; i < model.triangles_len; i++) {
            auto triangle = model.triangles[i];

            vec2i screen_coords[3];
            vec3f world_coords[3];
            for (int j = 0; j < 3; j++) {
                auto world = triangle.vertices[j].position;
                screen_coords[j] = vec2i{int32_t((world[0] + 1.0) * WIDTH / 2.0),
                                         int32_t((world[1] + 1.0) * HEIGHT / 2.0)};
                world_coords[j] = {world[0], world[1], world[2]};
            }

            vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
            float intensity = light_dir * n.normalize();
            if (intensity > 0) {
                frame.triangle(screen_coords[0], screen_coords[1], screen_coords[2],
                               {uint8_t(intensity * 255), uint8_t(intensity * 255), uint8_t(intensity * 255)});
            }
        }

        std::copy(place, place + len, buf);

        break;

        // doom_key_down(DOOM_KEY_ENTER);
        // doom_key_up(DOOM_KEY_ENTER);
    }
}
