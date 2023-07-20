#pragma once

#include <chrono>

#include "events.hpp"

namespace imgv
{
namespace chr = std::chrono;
class clock
{
public:
  clock();
  auto now() const -> double;

  auto get_speed() const -> double;

  auto set_speed(double speed) -> void;

  auto seek_forwards(double time) -> void;

private:
  using underlying_clock = chr::steady_clock;
  using time_point = typename underlying_clock::time_point;
  time_point m_start;

  double m_speed {1.0}, m_offset {0.0};

  auto save_state() -> void;
  auto calc_now(time_point time) const -> double;
};

// clock that remembers state
class state_clock
{
public:
  state_clock() = default;

  auto update_play_pause(const play_pause_event& e) -> void;
  auto update_speed(const speed_event& e) -> void;
  auto update_seek(const seek_event& e) -> void;

  auto now() const -> double;

  auto rescale(double time) const -> double;

private:
  clock m_base;
  bool m_playing {true};
  double m_last_speed {1.0};
  double m_total_seek {0.0};
};
}  // namespace imgv
