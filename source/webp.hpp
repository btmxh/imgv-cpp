#pragma once

#include <fstream>
#include <iterator>

#include <webp/decode.h>
#include <webp/demux.h>

#include "texture_load_common.hpp"

namespace imgv
{
struct webp_loader
{
  struct decoder_deleter
  {
    auto operator()(WebPAnimDecoder* d) { WebPAnimDecoderDelete(d); }
  };

  using decoder_t = unique_ptr<WebPAnimDecoder, decoder_deleter>;
  vector<char> data {};
  WebPData webp_data {};
  decoder_t decoder;
  WebPAnimInfo info {};
  image_metadata metadata {};
  vector<double> delays;

  explicit webp_loader(const char* path)
  {
    try {
      std::ifstream stream {path, std::ios::binary};
      auto size = [&]
      {
        stream.seekg(0, std::ios_base::end);
        auto sz = stream.tellg();
        stream.seekg(0, std::ios_base::beg);
        return static_cast<usize>(std::max<std::streampos>(sz, 0));
      }();
      stream.exceptions(std::ios_base::badbit);
      data.resize(size);
      auto read_size =
          stream.readsome(data.data(), static_cast<std::streampos>(size));
      data.resize(static_cast<usize>(read_size));
      if (data.size() == size) {
        while (!stream.eof()) {
          char ch = 0;
          stream.read(&ch, 1);
          data.push_back(ch);
        }
      }
    } catch (std::exception&) {
      IMGV_ERROR("unable to open and read file");
    }

    WebPDataInit(&webp_data);
    webp_data.bytes = reinterpret_cast<u8*>(data.data());
    webp_data.size = data.size();
    WebPAnimDecoderOptions options {};
    if (!WebPAnimDecoderOptionsInit(&options)) {
      IMGV_ERROR("unable to init decoder config");
    }

    options.color_mode = MODE_RGBA;

    decoder = decoder_t {WebPAnimDecoderNew(&webp_data, &options)};
    if (!decoder) {
      IMGV_ERROR("unable to allocate decoder");
    }

    if (!WebPAnimDecoderGetInfo(decoder.get(), &info)) {
      IMGV_ERROR("unable to get general media info");
    }

    metadata.width = static_cast<int>(info.canvas_width);
    metadata.height = static_cast<int>(info.canvas_height);
    metadata.title = path;
    metadata.animated = info.frame_count != 1;
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
                          static_cast<GLsizei>(metadata.width),
                          static_cast<GLsizei>(metadata.height),
                          static_cast<GLsizei>(info.frame_count),
                          0,
                          GL_RGBA,
                          GL_UNSIGNED_BYTE,
                          nullptr);

            while (WebPAnimDecoderHasMoreFrames(decoder.get())) {
              int timestamp = 0;
              u8* pixels = nullptr;
              WebPAnimDecoderGetNext(decoder.get(), &pixels, &timestamp);
              gl.TexSubImage3D(target,
                               0,
                               0,
                               0,
                               num_frame++,
                               static_cast<GLsizei>(metadata.width),
                               static_cast<GLsizei>(metadata.height),
                               1,
                               GL_RGBA,
                               GL_UNSIGNED_BYTE,
                               pixels);
              delays.push_back(timestamp * 1e-3);
            }
          } else {
            int timestamp = 0;
            u8* pixels = nullptr;
            WebPAnimDecoderGetNext(decoder.get(), &pixels, &timestamp);
            gl.TexImage2D(target,
                          0,
                          GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
                          static_cast<GLsizei>(metadata.width),
                          static_cast<GLsizei>(metadata.height),
                          0,
                          GL_RGBA,
                          GL_UNSIGNED_BYTE,
                          pixels);
          }

          gen_mipmap_and_set_filters(gl, target);
          dump_texture_compress_size(
              gl,
              sizeof(decltype(*std::declval<EasyGifReader::Frame>().pixels()))
                  * static_cast<usize>(metadata.width * metadata.height)
                  * info.frame_count,
              target);
          return texture;
        });
  }

  auto take_delays() -> vector<double> { return move(delays); }
};
}  // namespace imgv
