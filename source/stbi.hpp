#include <cassert>

#include <stb_image.hpp>

#include "texture_load_common.hpp"

namespace imgv
{

struct stbi_loader
{
  struct stbi_deleter
  {
    auto operator()(stbi_uc* ptr) { stbi_image_free(ptr); }
  };

  using pixel_data = unique_ptr<stbi_uc[], stbi_deleter>;

  image_metadata metadata;
  int num_comps = 0;
  pixel_data data;
  GLint internal_format, align;
  GLenum format;

  explicit stbi_loader(const char* path)
      : metadata {false, 0, 0, path}
      , data {stbi_load(
            path, &metadata.width, &metadata.height, &num_comps, STBI_default)}
  {
    if (!data) {
      throw runtime_error("unable to load image via stb_image");
    }

    switch (num_comps) {
      case 1:
        internal_format = GL_R8;
        format = GL_RED;
        align = 1;
        break;
      case 2:
        internal_format = GL_RG16;
        format = GL_RG;
        align = 2;
        break;
      case 3:
        internal_format = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
        format = GL_RGB;
        align = 1;
        break;
      case 4:
        internal_format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        format = GL_RGBA;
        align = 4;
        break;
      default:
        throw runtime_error("invalid num_comps");
    }
  }

  auto operator()(window* w) -> gl_texture
  {
    return w->use_gl(
        [&, this](const GladGLContext& gl)
        {
          auto texture = gl_texture::create(w);
          gl.BindTexture(GL_TEXTURE_2D, *texture);
          gl.PixelStorei(GL_UNPACK_ALIGNMENT, align);
          gl.TexImage2D(GL_TEXTURE_2D,
                        0,
                        internal_format,
                        metadata.width,
                        metadata.height,
                        0,
                        format,
                        GL_UNSIGNED_BYTE,
                        data.get());

          auto swizzle = [&](std::initializer_list<GLint> mask)
          {
            assert(mask.size() == 4);
            gl.TexParameteriv(
                GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, mask.begin());
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

          gen_mipmap_and_set_filters(gl, GL_TEXTURE_2D);

          if (num_comps >= 2) {
            dump_texture_compress_size(gl,
                                       static_cast<usize>(metadata.width)
                                           * static_cast<usize>(metadata.height)
                                           * static_cast<usize>(num_comps),
                                       GL_TEXTURE_2D);
          }
          data.reset();
          return texture;
        });
  }

  // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
  auto take_delays() -> vector<double> { return {}; }
};
}  // namespace imgv
