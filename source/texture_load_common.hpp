#pragma once

#include "gl_wrapper.hpp"

namespace imgv
{

inline auto gen_mipmap_and_set_filters(const GladGLContext& gl,
                                       GLenum tex_target) -> void
{
  gl.GenerateMipmap(tex_target);
  gl.TexParameteri(tex_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  gl.TexParameteri(
      tex_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
}

inline auto dump_texture_compress_size(const GladGLContext& gl,
                                       usize orig_size,
                                       GLenum tex_target) -> void
{
  GLint size = 0;
  gl.GetTexLevelParameteriv(
      tex_target, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &size);
  fmt::print("texture compress memory usage: {} -> {}\n", orig_size, size);
}

struct image_metadata
{
  bool animated;
  int width, height;
  const char* title;
};

}  // namespace imgv
