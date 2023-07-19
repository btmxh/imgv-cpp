#pragma once

#include <deque>

#include <mpv/client.h>
#include <mpv/render.h>

#include "types.hpp"

namespace imgv
{

class window;

struct windowed_event
{
  weak_ptr<window> handler_window;

  auto handler() const -> optional<shared_ptr<window>>
  {
    return handler_window.lock();
  }
};

struct mpv_render_update_event : public windowed_event
{
};

enum class change_mode
{
  set,
  add_or_cycle,
};

template<typename T>
struct change_event
{
  change_mode mode;
  T value;

  template<typename U>
  auto update(U& property)
  {
    switch (mode) {
      case change_mode::set:
        property = static_cast<U>(value);
      case change_mode::add_or_cycle:
        property = static_cast<U>(value + mode);
      default:;
    }
  }
};

struct play_pause_event
    : public windowed_event
    , change_event<bool>
{
};

struct speed_event
    : public windowed_event
    , change_event<double>
{
};

struct seek_event
    : public windowed_event
    , change_event<double>
{
};

using event =
    variant<mpv_render_update_event, play_pause_event, speed_event, seek_event>;

inline auto handler(event& e) -> optional<shared_ptr<window>>
{
  return visit([](auto& ev) { return ev.handler(); }, e);
}

class event_queue
{
public:
  event_queue() = default;

  auto push(event e) -> void;
  auto pop() -> optional<event>;

  template<typename T, typename... Args>
  auto emplace(Args&&... args) -> void
  {
    event e = T {forward<Args>(args)...};
    push(move(e));
  }

private:
  std::mutex m_mutex;
  std::deque<event> m_events {};
};

using weak_event_queue = weak_ptr<event_queue>;
using shared_event_queue = shared_ptr<event_queue>;

}  // namespace imgv
