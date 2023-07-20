#include <cmath>

#include "window.hpp"

#include <fmt/core.h>

#include "root_window.hpp"

#ifdef IMGV_X11
#  define GLFW_EXPOSE_NATIVE_X11
#  include <GLFW/glfw3native.h>
#  include <X11/Xatom.h>
#  include <X11/Xlib.h>
#endif

namespace imgv
{

auto glfw_window_deleter::operator()(GLFWwindow* window) noexcept -> void
{
  glfwDestroyWindow(window);
}

static auto set_x11_window_mode(GLFWwindow* window) -> void
{
#ifdef IMGV_X11
  auto* const x11_display = glfwGetX11Display();
  const auto x11_window = glfwGetX11Window(window);
  const auto window_type = XInternAtom(x11_display, "_NET_WM_WINDOW_TYPE", 0);
  const auto splash = XInternAtom(x11_display, "_NET_WM_WINDOW_TYPE_SPLASH", 0);
  XChangeProperty(x11_display,
                  x11_window,
                  window_type,
                  XA_ATOM,
                  32,
                  PropModeReplace,
                  reinterpret_cast<_Xconst unsigned char*>(&splash),
                  1);
#endif
}

window::window(weak_event_queue queue)
    : m_root(root_window::get())
    , m_queue {move(queue)}
{
  glfwDefaultWindowHints();
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
  glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
  glfwWindowHintString(GLFW_X11_CLASS_NAME, "imgv");
  glfwWindowHintString(GLFW_X11_INSTANCE_NAME, "imgv");
  m_window_handle = glfw_window {glfwCreateWindow(default_size,
                                                  default_size,
                                                  "imgv temp title",
                                                  nullptr,
                                                  m_root->get_glfw_handle())};
  set_x11_window_mode(m_window_handle.get());
  if (!m_window_handle) {
    throw std::runtime_error("unable to create window");
  }

  make_context_current();
  glfwSwapInterval(1);
  if (!gladLoadGLContext(&m_gl, glfwGetProcAddress)) {
    throw std::runtime_error("unable to load OpenGL function pointers");
  }

  glfwSetWindowUserPointer(m_window_handle.get(), this);
  glfwSetWindowRefreshCallback(
      m_window_handle.get(),
      [](GLFWwindow* w) {
        reinterpret_cast<window*>(glfwGetWindowUserPointer(w))->m_redraw = true;
      });
  glfwSetWindowCloseCallback(
      m_window_handle.get(),
      [](GLFWwindow* w)
      {
        reinterpret_cast<window*>(glfwGetWindowUserPointer(w))->m_dead = true;
        glfwHideWindow(w);
      });
  glfwSetKeyCallback(
      m_window_handle.get(),
      [](GLFWwindow* w, int key, int, int action, int)
      {
        auto& self = *reinterpret_cast<window*>(glfwGetWindowUserPointer(w));
        if (action == GLFW_RELEASE) {
          return;
        }
        if (key == GLFW_KEY_RIGHT) {
          self.push_event(imgv::seek_event {{self.weak_from_this()},
                                            {change_mode::add_or_cycle, 10.0}});
        } else if (key == GLFW_KEY_LEFT) {
          self.push_event(imgv::seek_event {
              {self.weak_from_this()}, {change_mode::add_or_cycle, -10.0}});
        } else if (key == GLFW_KEY_LEFT_BRACKET) {
          self.push_event(imgv::speed_event {
              {self.weak_from_this()}, {change_mode::add_or_cycle, -0.1}});
        } else if (key == GLFW_KEY_RIGHT_BRACKET) {
          self.push_event(imgv::speed_event {{self.weak_from_this()},
                                             {change_mode::add_or_cycle, 0.1}});
        } else if (key == GLFW_KEY_SPACE) {
          self.push_event(imgv::play_pause_event {
              {self.weak_from_this()}, {change_mode::add_or_cycle, true}});
        }
      });
  glfwSetMouseButtonCallback(
      m_window_handle.get(),
      [](GLFWwindow* w, int button, int action, int)
      {
        auto& self = *reinterpret_cast<window*>(glfwGetWindowUserPointer(w));
        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
          self.push_event(play_pause_event {
              {self.weak_from_this()},
              {change_mode::add_or_cycle, true},
          });
        } else if (button == GLFW_MOUSE_BUTTON_LEFT) {
          if (action == GLFW_PRESS) {
            self.m_drag_state.holding = true;
            std::tie(self.m_drag_state.ox, self.m_drag_state.oy) =
                window_drag_state::get_monitor_cursor_pos(w);
          } else if (action == GLFW_RELEASE) {
            self.m_drag_state.holding = false;
            self.m_drag_state.ox = 0;
            self.m_drag_state.oy = 0;
          }
        }
      });
  glfwSetCursorPosCallback(
      m_window_handle.get(),
      [](GLFWwindow* w, double, double)
      {
        auto& self = *reinterpret_cast<window*>(glfwGetWindowUserPointer(w));
        if (self.m_drag_state.holding) {
          auto&& [x, y] = window_drag_state::get_monitor_cursor_pos(w);
          self.m_drag_state.dx = x - self.m_drag_state.ox;
          self.m_drag_state.dy = y - self.m_drag_state.oy;
        }
      });
}

auto window::push_event(event e) -> void
{
  if (auto queue = m_queue.lock(); queue != nullptr) {
    queue->push(move(e));
  }
}

auto window::dead() const -> bool
{
  return m_dead;
}

auto window::make_context_current() const -> void
{
  glfwMakeContextCurrent(m_window_handle.get());
}

auto window::make_context_non_current() -> void
{
  glfwMakeContextCurrent(nullptr);
}

auto window::swap_buffers() const -> void
{
  glfwSwapBuffers(m_window_handle.get());
}

auto window_drag_state::update(GLFWwindow* window) -> void
{
  if (holding) {
    int x = 0, y = 0;
    glfwGetWindowPos(window, &x, &y);
    glfwSetWindowPos(window, x + static_cast<int>(std::floor(dx)), y + static_cast<int>(std::floor(dy)));
    ox += dx;
    oy += dy;
    dx = dy = 0;
  }
}

auto window_drag_state::get_window_pos(GLFWwindow* window) -> tuple<int, int>
{
  tuple<int, int> ret;
  glfwGetWindowPos(window, &std::get<0>(ret), &std::get<1>(ret));
  return ret;
}

auto window_drag_state::get_cursor_pos(GLFWwindow* window)
    -> tuple<double, double>
{
  tuple<double, double> ret;
  glfwGetCursorPos(window, &std::get<0>(ret), &std::get<1>(ret));
  return ret;
}

auto window_drag_state::get_monitor_cursor_pos(GLFWwindow* window)
    -> tuple<double, double>
{
  auto&& [wx, wy] = get_window_pos(window);
  auto&& [cx, cy] = get_cursor_pos(window);
  return std::make_tuple(wx + cx, wy + cy);
}

}  // namespace imgv
