template <size_t D>
using Color = std::array<uint8_t, D>;

struct Shader {
  virtual ~Shader() = default;

  virtual vec4f vertex(int iface, int nvert) = 0;
  virtual bool fragment(vec3f bar, Color<3> color) = 0;
};
