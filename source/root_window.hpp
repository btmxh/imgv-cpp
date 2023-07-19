#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "glad/gl.h"

#include "window.hpp"

namespace imgv
{

// NOLINTNEXTLINE(*-special-member-functions)
class glfw_context
{
public:
  glfw_context();
  ~glfw_context();
};

class root_window
{
public:
  root_window();

  auto get_glfw_handle() -> GLFWwindow*;

  static auto get() -> shared_ptr<root_window>;

private:
  glfw_context m_glfw;
  glfw_window m_window;
};
}  // namespace imgv
