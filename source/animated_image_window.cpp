#include <cassert>
#include <cmath>

#include "animated_image_window.hpp"

namespace imgv
{
const GLchar* const animated_fragment_shader = R"(
  #version 430 core

  layout(location = 0) in vec2 tex_coords;
  layout(location = 0) out vec4 color;

  layout(location = 0) uniform float layer;

  layout(binding = 0) uniform sampler2DArray tex;

  void main() {
    color = texture(tex, vec3(tex_coords, layer));
  }
)";

auto animated_image_window::handle_event(event& e) -> void
{
  visit(
      overloaded {
          [this](const play_pause_event& ev) { m_clock.update_play_pause(ev); },
          [this](const speed_event& ev) { m_clock.update_speed(ev); },
          [this](const seek_event& ev) { m_clock.update_seek(ev); },
          [](const auto&) {},
      },
      e);
}

auto animated_image_window::render() -> double
{
  // NOLINTNEXTLINE(bugprone-parent-virtual-call)
  window::render();
  auto time = std::fmod(m_clock.now(), m_delays.back());
  auto itr = std::lower_bound(m_delays.begin(), m_delays.end(), time);
  auto frame = std::distance(m_delays.begin(), itr);
  assert(frame >= 0);
  auto u_frame = static_cast<usize>(frame);

  if (u_frame != m_current_frame) {
    m_redraw = true;
  }

  m_current_frame = u_frame;

  if (!m_redraw) {
    // returns time until next frame
    return m_clock.rescale(*itr - time);
  }

  make_context_current();
  int width = 0, height = 0;
  glfwGetFramebufferSize(m_window_handle.get(), &width, &height);
  m_gl.Viewport(0, 0, width, height);
  m_gl.UseProgram(*m_program);
  m_gl.BindVertexArray(*m_vao);
  m_gl.ActiveTexture(GL_TEXTURE0);
  m_gl.BindTexture(GL_TEXTURE_2D_ARRAY, *m_texture);
  m_gl.Uniform1f(0, static_cast<GLfloat>(m_current_frame));
  m_gl.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  swap_buffers();

  return -1;
}
}  // namespace imgv
