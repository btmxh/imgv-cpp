#pragma once

#include <mpv/client.h>
#include <mpv/render.h>
#include <mpv/render_gl.h>

#include "window.hpp"

namespace imgv
{
struct mpv_handle_deleter
{
  auto operator()(mpv_handle* handle) { mpv_destroy(handle); }
};

struct mpv_render_context_deleter
{
  auto operator()(mpv_render_context* context)
  {
    mpv_render_context_free(context);
  }
};

class mpv_window : public window
{
public:
  mpv_window(weak_event_queue queue, const char* path);
  ~mpv_window() override = default;

  mpv_window(const mpv_window&) = delete;
  mpv_window(mpv_window&&) = delete;

  auto operator=(const mpv_window&) = delete;
  auto operator=(mpv_window&&) = delete;

  auto handle_event(event& e) -> void override;
  auto update() -> void override;
  auto render() -> bool override;

private:
  unique_ptr<mpv_handle, mpv_handle_deleter> m_mpv;
  unique_ptr<mpv_render_context, mpv_render_context_deleter> m_render;
  i32 m_width = 0, m_height = 0;

  auto try_show() -> void;
  auto handle_mpv_events() -> void;
  // unique_ptr<
};
}  // namespace imgv
