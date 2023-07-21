#pragma once

#include "gl_wrapper.hpp"
#include "window.hpp"

namespace imgv
{

static constexpr const char generic_vertex_shader[] = R"(
  #version 430 core

  layout(location = 0) out vec2 tex_coords;

  const vec2 vertices[4] = vec2[](
    vec2(-1,1), vec2(1,1), vec2(-1,-1), vec2(1,-1)
  );
  void main() {
    gl_Position = vec4(vertices[gl_VertexID], 0.0, 1.0);
    tex_coords = vertices[gl_VertexID] * vec2(0.5, -0.5) + vec2(0.5, 0.5);
  }
)";

class gl_window : public window
{
public:
  explicit gl_window(weak_event_queue queue);
  ~gl_window() override = default;

  gl_window(const gl_window&) = delete;
  gl_window(gl_window&&) = delete;

  auto operator=(const gl_window&) = delete;
  auto operator=(gl_window&&) = delete;

  auto render() -> double override;

  virtual auto render_frame() -> void {}

protected:
  auto gen_mipmap_and_set_filters(GLenum tex_target) -> void;
  auto dump_texture_compress_size(usize orig_size, GLenum tex_target) -> void;

  gl_vertex_array m_vao;
};
}  // namespace imgv
