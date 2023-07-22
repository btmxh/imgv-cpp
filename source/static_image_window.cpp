#include <cassert>
#include <initializer_list>

#include "static_image_window.hpp"

#include <stb_image.hpp>

namespace imgv
{
const GLchar* const static_vertex_shader = R"(
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

const GLchar* const static_fragment_shader = R"(
  #version 430 core

  layout(location = 0) in vec2 tex_coords;
  layout(location = 0) out vec4 color;

  layout(binding = 0) uniform sampler2D tex;

  void main() {
    color = texture(tex, tex_coords);
  }
)";

auto static_image_window::render() -> double
{
  window::render();

  if (!m_redraw) {
    return std::numeric_limits<double>::infinity();
  }

  make_context_current();
  int width = 0, height = 0;
  glfwGetFramebufferSize(m_window_handle.get(), &width, &height);
  m_gl.Viewport(0, 0, width, height);

  m_gl.UseProgram(*m_program);
  m_gl.BindVertexArray(*m_vao);
  m_gl.ActiveTexture(GL_TEXTURE0);
  m_gl.BindTexture(GL_TEXTURE_2D, *m_texture);
  m_gl.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  swap_buffers();

  return -1;
}

}  // namespace imgv
