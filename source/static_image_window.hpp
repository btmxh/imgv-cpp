#pragma once

#include "gl_wrapper.hpp"

namespace imgv
{

extern const GLchar* const static_vertex_shader;
extern const GLchar* const static_fragment_shader;

class static_image_window : public window
{
public:
  template<typename Loader>
  static_image_window(weak_event_queue queue,
                      Loader& loader,
                      const char* fragment_shader = static_fragment_shader)
      : window {move(queue)}
      , m_program {create_program(this, static_vertex_shader, fragment_shader)}
      , m_vao {gl_vertex_array::create(this)}
      , m_texture {[&, this]
                   {
                     make_context_current();
                     return loader(this);
                   }()}
  {
    show_window(
        loader.metadata.width, loader.metadata.height, loader.metadata.title);
  }
  ~static_image_window() override = default;

  static_image_window(const static_image_window&) = delete;
  static_image_window(static_image_window&&) = delete;

  auto operator=(const static_image_window&) = delete;
  auto operator=(static_image_window&&) = delete;

  auto render() -> double override;

protected:
  gl_program m_program;
  gl_vertex_array m_vao;
  gl_texture m_texture;
};
}  // namespace imgv
