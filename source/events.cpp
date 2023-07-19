#include "events.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "mpv_window.hpp"

namespace imgv
{

auto event_queue::push(event e) -> void
{
  const scoped_lock lock {m_mutex};
  m_events.push_back(move(e));
  glfwPostEmptyEvent();
}

auto event_queue::pop() -> optional<event>
{
  const scoped_lock lock {m_mutex};
  if (m_events.empty()) {
    return nullopt;
  }

  auto e = m_events.front();
  m_events.pop_front();
  return e;
}

}  // namespace imgv
