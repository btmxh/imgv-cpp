#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>

#include "events.hpp"
#include "types.hpp"

#include <atomic>

namespace imgv
{

struct glfw_window_deleter
{
  auto operator()(GLFWwindow* window) noexcept -> void;
};

using glfw_window = unique_ptr<GLFWwindow, glfw_window_deleter>;

class root_window;

class window : public std::enable_shared_from_this<window>
{
public:
  constexpr static i32 default_size = 16;
  explicit window(weak_event_queue queue);
  virtual ~window() = default;

  window(const window&) = delete;
  window(window&&) = delete;

  auto operator=(const window&) = delete;
  auto operator=(window&&) = delete;

  virtual auto handle_event(event& /*e*/) -> void {}
  virtual auto update() -> void {}
  virtual auto render() -> bool { return false; }

  auto push_event(event e) -> void;
  auto dead() const -> bool;

protected:
  auto make_context_current() -> void;
  static auto make_context_non_current() -> void;
  auto swap_buffers() -> void;

  shared_ptr<root_window> m_root;
  weak_event_queue m_queue;
  glfw_window m_window_handle;
  std::atomic_bool m_redraw {true}, m_dead {false};
  GladGLContext m_gl {};
};
}  // namespace imgv
