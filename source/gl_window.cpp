#include <cmath>

#include "gl_window.hpp"

#include <fmt/core.h>

namespace imgv
{

gl_window::gl_window(weak_event_queue queue)
    : window {move(queue)}
    , m_vao {gl_vertex_array::create(this)}
{
}

auto gl_window::render() -> double
{
  window::render();

  if (!m_redraw) {
    return std::numeric_limits<double>::infinity();
  }

  make_context_current();
  int width = 0, height = 0;
  glfwGetFramebufferSize(m_window_handle.get(), &width, &height);
  m_gl.Viewport(0, 0, width, height);

  render_frame();
  swap_buffers();

  return -1;
}
auto gl_window::gen_mipmap_and_set_filters(GLenum tex_target) -> void
{
  m_gl.GenerateMipmap(tex_target);
  m_gl.TexParameteri(tex_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  m_gl.TexParameteri(
      tex_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
}

auto gl_window::dump_texture_compress_size(usize orig_size, GLenum tex_target)
    -> void
{
  GLint size = 0;
  m_gl.GetTexLevelParameteriv(
      tex_target, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &size);
  fmt::print("texture compress memory usage: {} -> {}\n", orig_size, size);
}
}  // namespace imgv
