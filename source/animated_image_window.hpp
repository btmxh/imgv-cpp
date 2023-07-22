#pragma once

#include <limits>
#include <type_traits>

#include <EasyGifReader.h>

#include "clock.hpp"
#include "gl_wrapper.hpp"
#include "static_image_window.hpp"

namespace imgv
{
extern const GLchar* const animated_fragment_shader;

class animated_image_window : public static_image_window
{
public:
  template<typename Func,
           typename = std::enable_if_t<std::is_rvalue_reference_v<Func&&>>>
  animated_image_window(weak_event_queue queue, Func&& loader)
      : static_image_window {move(queue), loader, animated_fragment_shader}
      , m_delays {loader.take_delays()}
  {
  }

  ~animated_image_window() override = default;

  animated_image_window(const animated_image_window&) = delete;
  animated_image_window(animated_image_window&&) = delete;

  auto operator=(const animated_image_window&) = delete;
  auto operator=(animated_image_window&&) = delete;

  auto handle_event(event& e) -> void override;
  auto render() -> double override;

private:
  vector<double> m_delays;
  state_clock m_clock;
  usize m_current_frame {std::numeric_limits<usize>::max()};
};
}  // namespace imgv
