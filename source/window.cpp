#include "window.hpp"

#include <fmt/core.h>

#include "root_window.hpp"

namespace imgv
{

auto glfw_window_deleter::operator()(GLFWwindow* window) noexcept -> void
{
  glfwDestroyWindow(window);
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
        if (action != GLFW_PRESS) {
          return;
        }
        if (button == GLFW_MOUSE_BUTTON_RIGHT) {
          self.push_event(play_pause_event {
              {self.weak_from_this()},
              {change_mode::add_or_cycle, true},
          });
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

auto window::make_context_current() -> void
{
  glfwMakeContextCurrent(m_window_handle.get());
}

auto window::make_context_non_current() -> void
{
  glfwMakeContextCurrent(nullptr);
}

auto window::swap_buffers() -> void
{
  glfwSwapBuffers(m_window_handle.get());
}

}  // namespace imgv
