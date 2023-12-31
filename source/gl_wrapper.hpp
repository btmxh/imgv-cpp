#pragma once

#include <fmt/core.h>
#include <glad/gl.h>

#include "types.hpp"
#include "window.hpp"

namespace imgv
{
template<typename Trait>
class gl_object
{
public:
  using handle_t = typename Trait::handle_t;

  explicit gl_object(std::nullptr_t = nullptr)
      : m_owner {nullptr}
      , m_handle {Trait::null_handle()}
  {
  }

  explicit gl_object(window* owner, handle_t handle)
      : m_owner {owner}
      , m_handle {handle}
  {
  }

  ~gl_object() { reset(); }

  gl_object(const gl_object&) = delete;
  auto operator=(const gl_object&) = delete;

  gl_object(gl_object&& other) noexcept
      : m_owner {std::exchange(other.m_owner, nullptr)}
      , m_handle {std::exchange(other.m_handle, 0)}
  {
  }

  auto operator=(gl_object&& other) noexcept -> gl_object&
  {
    reset();
    swap(*this, other);
    return *this;
  }

  friend auto swap(gl_object& lhs, gl_object& rhs) -> void
  {
    using std::swap;
    swap(lhs.m_handle, rhs.m_handle);
    swap(lhs.m_owner, rhs.m_owner);
  }

  auto release() -> handle_t
  {
    return std::exchange(m_handle, Trait::null_handle());
  }

  auto reset() -> void
  {
    if (auto handle = release(); handle != Trait::null_handle()) {
      Trait::destroy(*m_owner, handle);
    }
  }

  auto get() const -> handle_t { return m_handle; }

  auto operator*() const -> handle_t { return get(); }

  template<typename... Args>
  static auto create(window* owner, Args&&... args) -> gl_object
  {
    owner->make_context_current();
    auto handle = Trait::create(*owner, forward<Args>(args)...);
    if (handle == Trait::null_handle()) {
      IMGV_ERROR(fmt::format("unable to create {} object", Trait::type_name()));
    }

    return gl_object {owner, handle};
  }

private:
  window* m_owner;
  handle_t m_handle;
};

struct gl_common_trait
{
  using handle_t = GLuint;

  static auto null_handle() -> handle_t { return 0; }
};

// handle_t is not visible in this scope
// NOLINTNEXTLINE(*-macro-usage)
#define IMGV_GLGEN(fn, ...) \
  do { \
    handle_t h; \
    fn(__VA_ARGS__, &h); \
    return h; \
  } while (0);

struct vertex_array_trait : gl_common_trait
{
  static auto type_name() -> const char* { return "vertex array"; }
  static constexpr auto destroy = [](const window& w, handle_t vao)
  { w.use_gl([&](const auto& gl) { gl.DeleteVertexArrays(1, &vao); }); };
  static constexpr auto create = [](const window& w) {
    return w.use_gl([](const auto& gl) { IMGV_GLGEN(gl.GenVertexArrays, 1); });
  };
};

struct texture_trait : gl_common_trait
{
  static auto type_name() -> const char* { return "texture"; }
  static constexpr auto destroy = [](const window& w, handle_t vao)
  { w.use_gl([&](const auto& gl) { gl.DeleteTextures(1, &vao); }); };
  static constexpr auto create = [](const window& w)
  { return w.use_gl([](const auto& gl) { IMGV_GLGEN(gl.GenTextures, 1); }); };
};
struct program_trait : gl_common_trait
{
  static auto type_name() -> const char* { return "shader program"; }
  static constexpr auto destroy = [](const window& w, handle_t prog)
  { w.use_gl([=](const auto& gl) { gl.DeleteProgram(prog); }); };
  static constexpr auto create = [](const window& w)
  { return w.use_gl([](const auto& gl) { return gl.CreateProgram(); }); };
};
struct shader_trait : gl_common_trait
{
  static auto type_name() -> const char* { return "shader"; }
  static constexpr auto destroy = [](const window& w, handle_t shader)
  { w.use_gl([=](const auto& gl) { gl.DeleteShader(shader); }); };
  static constexpr auto create = [](const window& w, GLenum type)
  { return w.use_gl([=](const auto& gl) { return gl.CreateShader(type); }); };
};

using gl_vertex_array = gl_object<vertex_array_trait>;
using gl_texture = gl_object<texture_trait>;
using gl_program = gl_object<program_trait>;
using gl_shader = gl_object<shader_trait>;

inline auto create_shader(window* owner, GLenum type, const GLchar* source)
    -> gl_shader
{
  return owner->use_gl(
      [=](const GladGLContext& gl)
      {
        auto shader = gl_shader::create(owner, type);
        gl.ShaderSource(*shader, 1, &source, nullptr);
        gl.CompileShader(*shader);

        GLint i = 0;
        gl.GetShaderiv(*shader, GL_COMPILE_STATUS, &i);
        if (i == GL_FALSE) {
          GLchar buf[256];
          gl.GetShaderInfoLog(*shader, sizeof(buf), nullptr, buf);
          IMGV_ERROR(fmt::format("unable to compile shader: {}", buf));
        }

        return shader;
      });
}

inline auto create_program(window* owner,
                           const GLchar* vs_src,
                           const GLchar* fs_src) -> gl_program
{
  return owner->use_gl(
      [=](const GladGLContext& gl)
      {
        auto vs = create_shader(owner, GL_VERTEX_SHADER, vs_src);
        auto fs = create_shader(owner, GL_FRAGMENT_SHADER, fs_src);

        auto program = gl_program::create(owner);
        gl.AttachShader(*program, *vs);
        gl.AttachShader(*program, *fs);
        gl.LinkProgram(*program);
        GLint i = 0;
        gl.GetProgramiv(*program, GL_LINK_STATUS, &i);
        if (i == GL_FALSE) {
          GLchar buf[256];
          gl.GetProgramInfoLog(*program, sizeof(buf), nullptr, buf);
          IMGV_ERROR(fmt::format("unable to link shader program: {}", buf));
        }

        gl.DetachShader(*program, *vs);
        gl.DetachShader(*program, *fs);

        return program;
      });
}

}  // namespace imgv
