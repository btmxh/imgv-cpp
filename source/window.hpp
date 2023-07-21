#pragma once

#define GLFW_INCLUDE_NONE
#include <atomic>
#include <functional>

#include <GLFW/glfw3.h>
#include <glad/gl.h>

#include "events.hpp"
#include "types.hpp"

namespace imgv
{

struct glfw_window_deleter
{
  auto operator()(GLFWwindow* window) noexcept -> void;
};

using glfw_window = unique_ptr<GLFWwindow, glfw_window_deleter>;

class root_window;

struct window_drag_state
{
  bool holding {false};
  // original position (wrt monitor)
  double ox {0}, oy {0};
  // delta position
  double dx {0}, dy {0};

  auto update(GLFWwindow* window) -> void;
  static auto get_window_pos(GLFWwindow* window) -> tuple<int, int>;
  static auto get_cursor_pos(GLFWwindow* window) -> tuple<double, double>;
  static auto get_monitor_cursor_pos(GLFWwindow* window)
      -> tuple<double, double>;
};

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
  // return the wait time
  // <0 indicates vsync
  virtual auto render() -> double
  {
    m_drag_state.update(m_window_handle.get());
    return 0.05;
  }

  auto push_event(event e) -> void;
  auto dead() const -> bool;

  template<typename Func>
  auto use_gl(Func&& func) const -> decltype(auto)
  {
    make_context_current();
    return std::invoke(forward<Func>(func), m_gl);
  }

  auto make_context_current() const -> void;
  static auto make_context_non_current() -> void;

protected:
  auto swap_buffers() const -> void;

  auto show_window(int width, int height, const char* title) -> void;

  shared_ptr<root_window> m_root;
  weak_event_queue m_queue;
  glfw_window m_window_handle;
  std::atomic_bool m_redraw {true}, m_dead {false};
  GladGLContext m_gl {};
  window_drag_state m_drag_state {};
};

auto create_window(weak_event_queue queue, const char* path) -> shared_ptr<window>;
}  // namespace imgv
