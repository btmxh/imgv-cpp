#include <mutex>

#include "root_window.hpp"

#include <fmt/core.h>

namespace imgv
{

root_window::root_window()
    : m_window {[]
                {
                  glfwDefaultWindowHints();
                  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
                  return glfwCreateWindow(window::default_size,
                                          window::default_size,
                                          "imgv root window",
                                          nullptr,
                                          nullptr);
                }()}
{
  glfwSetErrorCallback(
      [](int error, const char* message)
      { fmt::print(stderr, "GLFW error {}: {}\n", error, message); });
}

auto root_window::get_glfw_handle() -> GLFWwindow*
{
  return m_window.get();
}

auto root_window::get() -> shared_ptr<root_window>
{
  static weak_ptr<root_window> instance;
  static std::mutex mutex;

  const scoped_lock guard {mutex};
  auto ptr = instance.lock();
  if (!ptr) {
    ptr = std::make_shared<root_window>();
    instance = weak_ptr {ptr};
  }

  return ptr;
}

glfw_context::glfw_context()
{
  if (!glfwInit()) {
    IMGV_ERROR("unable to initialize GLFW");
  }
}

glfw_context::~glfw_context()
{
  glfwTerminate();
}

}  // namespace imgv
