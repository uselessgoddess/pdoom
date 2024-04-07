#include "types.h"

struct Vertex {
    f32 position[3];
};

struct Triangle {
    Vertex vertices[3];
};

struct ObjRepr {
    Triangle *triangles;
    size_t triangles_len;
};

extern "C" ObjRepr load_elemental();
