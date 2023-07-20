#include <cmath>
#include <limits>

#include "clock.hpp"

namespace imgv
{
clock::clock()
    : m_start {underlying_clock::now()}
{
}

auto clock::now() const -> double
{
  return calc_now(underlying_clock::now());
}

auto clock::get_speed() const -> double
{
  return m_speed;
}

auto clock::set_speed(double speed) -> void
{
  save_state();
  m_speed = speed;
}

auto clock::seek_forwards(double time) -> void
{
  m_offset += time;
}

auto clock::save_state() -> void
{
  auto now = underlying_clock::now();
  m_offset = calc_now(now);
  m_start = now;
}

auto clock::calc_now(time_point time) const -> double
{
  return m_offset
      + m_speed
      * chr::duration_cast<chr::duration<double>>(time - m_start).count();
}

auto state_clock::update_play_pause(const play_pause_event& e) -> void
{
  e.update(m_playing);
  m_base.set_speed(m_playing ? m_last_speed : 0.0);
}

auto state_clock::update_speed(const speed_event& e) -> void
{
  e.update(m_last_speed);
  if (m_playing) {
    m_base.set_speed(m_last_speed);
  }
}

auto state_clock::update_seek(const seek_event& e) -> void
{
  auto last_seek = m_total_seek;
  e.update(m_total_seek);
  m_base.seek_forwards(m_total_seek - last_seek);
}

auto state_clock::now() const -> double
{
  return m_base.now();
}

auto state_clock::rescale(double time) const -> double
{
  if (time <= 0) {
    return 0;
  }
  auto speed = m_base.get_speed();
  if (std::fabs(speed) < 1e-3) {
    return std::numeric_limits<double>::infinity();
  }

  return time / speed;
}

}  // namespace imgv
