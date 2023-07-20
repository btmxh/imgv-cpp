#pragma once

#include <EasyGifReader.h>

// #include "image_texture.hpp"
#include "clock.hpp"
#include "gl_wrapper.hpp"
#include "window.hpp"

#include <limits>

namespace imgv
{
class gif_window : public window
{
public:
  gif_window(weak_event_queue queue, const char* path);
  ~gif_window() override = default;

  gif_window(const gif_window&) = delete;
  gif_window(gif_window&&) = delete;

  auto operator=(const gif_window&) = delete;
  auto operator=(gif_window&&) = delete;

  auto handle_event(event& e) -> void override;
  auto render() -> double override;

private:
  gl_program m_program;
  gl_vertex_array m_vao;
  gl_texture m_tex;
  vector<double> m_delays;
  state_clock m_clock;
  usize m_current_frame {std::numeric_limits<usize>::max()};
};
}  // namespace imgv
