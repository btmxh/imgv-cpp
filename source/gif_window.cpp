#include <cmath>

#include "gif_window.hpp"

#include <fmt/core.h>

namespace imgv
{
static auto create_shader(window* owner, GLenum type, const GLchar* source)
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
          throw runtime_error(fmt::format("unable to compile shader: {}", buf));
        }

        return shader;
      });
}

static auto create_program(window* owner) -> gl_program
{
  return owner->use_gl(
      [=](const GladGLContext& gl)
      {
        auto vs = create_shader(owner, GL_VERTEX_SHADER, R"(
    #version 430 core

    layout(location = 0) out vec2 tex_coords;

    const vec2 vertices[4] = vec2[](
      vec2(-1,1), vec2(1,1), vec2(-1,-1), vec2(1,-1)
    );
    void main() {
      gl_Position = vec4(vertices[gl_VertexID], 0.0, 1.0);
      tex_coords = vertices[gl_VertexID] * vec2(0.5, -0.5) + vec2(0.5, 0.5);
    }
  )");
        auto fs = create_shader(owner, GL_FRAGMENT_SHADER, R"(
    #version 430 core

    layout(location = 0) in vec2 tex_coords;
    layout(location = 0) out vec4 color;

    layout(location = 0) uniform float layer;

    layout(binding = 0) uniform sampler2DArray tex;

    void main() {
      color = texture(tex, vec3(tex_coords, layer));
    }
  )");

        auto program = gl_program::create(owner);
        gl.AttachShader(*program, *vs);
        gl.AttachShader(*program, *fs);
        gl.LinkProgram(*program);
        GLint i = 0;
        gl.GetProgramiv(*program, GL_LINK_STATUS, &i);
        if (i == GL_FALSE) {
          GLchar buf[256];
          gl.GetProgramInfoLog(*program, sizeof(buf), nullptr, buf);
          throw runtime_error(
              fmt::format("unable to link shader program: {}", buf));
        }

        gl.DetachShader(*program, *vs);
        gl.DetachShader(*program, *fs);

        return program;
      });
}

gif_window::gif_window(weak_event_queue queue, const char* path)
    : window {move(queue)}
    , m_program {create_program(this)}
    , m_vao {gl_vertex_array::create(this)}
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

  m_gl.GenerateMipmap(GL_TEXTURE_2D_ARRAY);
  m_gl.TexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  m_gl.TexParameteri(
      GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);

  {
    GLint size = 0;
    m_gl.GetTexLevelParameteriv(
        GL_TEXTURE_2D_ARRAY, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &size);
    auto orig_size = static_cast<int>(sizeof(i32)) * reader.width()
        * reader.height() * reader.frameCount();
    fmt::print("texture compress memory usage: {} -> {}\n", orig_size, size);
  }

  glfwSetWindowSize(m_window_handle.get(), reader.width(), reader.height());
  glfwSetWindowAspectRatio(
      m_window_handle.get(), reader.width(), reader.height());
  glfwSetWindowTitle(m_window_handle.get(), path);
  glfwShowWindow(m_window_handle.get());
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
