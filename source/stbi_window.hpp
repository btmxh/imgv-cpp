#pragma once

#include "gl_window.hpp"

namespace imgv
{
class stbi_window : public gl_window
{
public:
  stbi_window(weak_event_queue queue, const char* path);
  ~stbi_window() override = default;

  stbi_window(const stbi_window&) = delete;
  stbi_window(stbi_window&&) = delete;

  auto operator=(const stbi_window&) = delete;
  auto operator=(stbi_window&&) = delete;

  auto render_frame() -> void override;

private:
  gl_program m_program;
  gl_texture m_texture;
};
}  // namespace imgv
