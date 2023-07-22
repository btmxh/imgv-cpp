#pragma once

#include <cassert>

#include <EasyGifReader.h>

#include "texture_load_common.hpp"
#include "types.hpp"

namespace imgv
{

struct gif_loader
{
  EasyGifReader reader;
  image_metadata metadata;
  vector<double> delays;

  explicit gif_loader(const char* path)
      : reader {EasyGifReader::openFile(path)}
      , metadata {
            reader.frameCount() != 1, reader.width(), reader.height(), path}
  {
  }

  auto operator()(window* w) -> gl_texture
  {
    return w->use_gl(
        [&, this](const GladGLContext& gl)
        {
          auto texture = gl_texture::create(w);
          const GLenum target =
              metadata.animated ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
          gl.BindTexture(target, *texture);
          if (metadata.animated) {
            GLsizei num_frame = 0;
            gl.TexImage3D(target,
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
              gl.TexSubImage3D(target,
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
              auto last_delay = delays.empty() ? 0.0 : delays.back();
              delays.push_back(frame.duration().seconds() + last_delay);
            }
          } else {
            auto frame_it = reader.begin();
            assert(frame_it != reader.end());
            gl.TexImage2D(target,
                          0,
                          GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
                          static_cast<GLsizei>(reader.width()),
                          static_cast<GLsizei>(reader.height()),
                          0,
                          GL_RGBA,
                          GL_UNSIGNED_BYTE,
                          frame_it->pixels());
          }
          gen_mipmap_and_set_filters(gl, target);
          dump_texture_compress_size(
              gl,
              sizeof(decltype(*std::declval<EasyGifReader::Frame>().pixels()))
                  * static_cast<usize>(reader.width() * reader.height()
                                       * reader.frameCount()),
              target);
          return texture;
        });
  }

  auto take_delays() -> vector<double> { return move(delays); }
};
}  // namespace imgv
