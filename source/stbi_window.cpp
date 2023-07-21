#include <cassert>
#include <initializer_list>

#include "stbi_window.hpp"

#include <stb_image.hpp>

namespace imgv
{
stbi_window::stbi_window(weak_event_queue queue, const char* path)
    : gl_window {move(queue)}
    , m_program {create_program(this, generic_vertex_shader, R"(
  #version 430 core

  layout(location = 0) in vec2 tex_coords;
  layout(location = 0) out vec4 color;

  layout(binding = 0) uniform sampler2D tex;

  void main() {
    color = texture(tex, tex_coords);
  }
)")}
    , m_texture {gl_texture::create(this)}
{
  struct stbi_deleter
  {
    auto operator()(stbi_uc* ptr) { stbi_image_free(ptr); }
  };

  int width = 0, height = 0, num_comps = 0;
  auto data = unique_ptr<stbi_uc[], stbi_deleter> {
      stbi_load(path, &width, &height, &num_comps, STBI_default)};
  if (!data) {
    throw runtime_error("unable to load image via stb_image");
  }
  auto&& [internal_format, format, align] = [=]() -> tuple<GLint, GLenum, GLint>
  {
    switch (num_comps) {
      case 1:
        return std::make_tuple(GL_R8, GL_RED, 1);
      case 2:
        return std::make_tuple(GL_RG16, GL_RG, 2);
      case 3:
        return std::make_tuple(GL_COMPRESSED_RGB_S3TC_DXT1_EXT, GL_RGB, 1);
      case 4:
        return std::make_tuple(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, GL_RGBA, 4);
      default:
        throw runtime_error("invalid num_comps");
    }
  }();

  make_context_current();
  m_gl.BindTexture(GL_TEXTURE_2D, *m_texture);
  m_gl.PixelStorei(GL_UNPACK_ALIGNMENT, align);
  m_gl.TexImage2D(GL_TEXTURE_2D,
                  0,
                  internal_format,
                  width,
                  height,
                  0,
                  format,
                  GL_UNSIGNED_BYTE,
                  data.get());

  auto swizzle = [&](std::initializer_list<GLint> mask)
  {
    assert(mask.size() == 4);
    m_gl.TexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, mask.begin());
  };

  switch (num_comps) {
    case 1:
      swizzle({GL_RED, GL_RED, GL_RED, GL_ONE});
      break;
    case 2:
      swizzle({GL_RED, GL_RED, GL_RED, GL_GREEN});
      break;
    default:;
  }

  gen_mipmap_and_set_filters(GL_TEXTURE_2D);

  if (num_comps >= 2) {
    dump_texture_compress_size(static_cast<usize>(width)
                                   * static_cast<usize>(height)
                                   * static_cast<usize>(num_comps),
                               GL_TEXTURE_2D);
  }
  data.reset();

  show_window(width, height, path);
}

auto stbi_window::render_frame() -> void
{
  m_gl.UseProgram(*m_program);
  m_gl.BindVertexArray(*m_vao);
  m_gl.ActiveTexture(GL_TEXTURE0);
  m_gl.BindTexture(GL_TEXTURE_2D, *m_texture);
  m_gl.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

}  // namespace imgv
