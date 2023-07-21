#include <cassert>
#include <cmath>

#include "gif_window.hpp"

#include <fmt/core.h>

namespace imgv
{
gif_window::gif_window(weak_event_queue queue, const char* path)
    : gl_window {move(queue)}
    , m_program {create_program(this,
                                generic_vertex_shader,
                                R"(
    #version 430 core

    layout(location = 0) in vec2 tex_coords;
    layout(location = 0) out vec4 color;

    layout(location = 0) uniform float layer;

    layout(binding = 0) uniform sampler2DArray tex;

    void main() {
      color = texture(tex, vec3(tex_coords, layer));
    }
  )")}
    , m_tex {gl_texture::create(this)}
{
  make_context_current();
  auto reader = EasyGifReader::openFile(path);

  m_gl.BindTexture(GL_TEXTURE_2D_ARRAY, *m_tex);

  GLsizei num_frame = 0;
  m_gl.TexImage3D(GL_TEXTURE_2D_ARRAY,
                  0,
                  GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
                  static_cast<GLsizei>(reader.width()),
                  static_cast<GLsizei>(reader.height()),
                  static_cast<GLsizei>(reader.frameCount()),
                  0,
                  GL_RGBA,
                  GL_UNSIGNED_BYTE,
                  nullptr);
  for (const auto& frame : reader) {
    m_gl.TexSubImage3D(GL_TEXTURE_2D_ARRAY,
                       0,
                       0,
                       0,
                       num_frame++,
                       static_cast<GLsizei>(frame.width()),
                       static_cast<GLsizei>(frame.height()),
                       1,
                       GL_RGBA,
                       GL_UNSIGNED_BYTE,
                       frame.pixels());
    auto last_delay = m_delays.empty() ? 0.0 : m_delays.back();
    m_delays.push_back(frame.duration().seconds() + last_delay);
  }

  gen_mipmap_and_set_filters(GL_TEXTURE_2D_ARRAY);
  dump_texture_compress_size(
      sizeof(decltype(*std::declval<EasyGifReader::Frame>().pixels()))
          * static_cast<usize>(reader.width() * reader.height()),
      GL_TEXTURE_2D_ARRAY);

  show_window(reader.width(), reader.height(), path);
}

auto gif_window::handle_event(event& e) -> void
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

auto gif_window::render() -> double
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
  m_gl.BindTexture(GL_TEXTURE_2D_ARRAY, *m_tex);
  m_gl.Uniform1f(0, static_cast<GLfloat>(m_current_frame));
  m_gl.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  swap_buffers();

  return -1;
}
}  // namespace imgv
