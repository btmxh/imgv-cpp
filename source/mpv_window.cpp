#include <cassert>

#include "mpv_window.hpp"

#include <fmt/core.h>
#include <mpv/client.h>
#include <mpv/render.h>
#include <mpv/render_gl.h>

#include "types.hpp"

namespace imgv
{
enum class reply_data : int
{
  video_out_params,
};

mpv_window::mpv_window(weak_event_queue queue, const char* path)
    : window(move(queue))
{
  glfwSetWindowTitle(m_window_handle.get(), path);
  m_mpv = decltype(m_mpv) {mpv_create()};
  int i_true = 1;
  if (mpv_set_option_string(m_mpv.get(), "input-default-bindings", "yes") < 0
      || mpv_set_option_string(m_mpv.get(), "input-vo-keyboard", "yes") < 0
      || mpv_set_option(m_mpv.get(), "osc", MPV_FORMAT_FLAG, &i_true))
  {
    throw runtime_error("unable to set mpv options");
  }
  if (!m_mpv || mpv_initialize(m_mpv.get()) < 0) {
    throw std::runtime_error("unable to create & initialize mpv handle");
  }

#ifdef NDEBUG
  mpv_request_log_messages(m_mpv.get(), "info");
#else
  mpv_request_log_messages(m_mpv.get(), "debug");
#endif
  mpv_opengl_init_params gl_params = {
      [](void*, const char* name)
      { return reinterpret_cast<void*>(glfwGetProcAddress(name)); },
      nullptr};

  mpv_render_param params[] = {
      {MPV_RENDER_PARAM_API_TYPE,
       const_cast<void*>(
           reinterpret_cast<const void*>(MPV_RENDER_API_TYPE_OPENGL))},
      {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_params},
      {MPV_RENDER_PARAM_ADVANCED_CONTROL, &i_true},
      {MPV_RENDER_PARAM_INVALID, nullptr}};

  mpv_render_context* context = nullptr;
  if (mpv_render_context_create(&context, m_mpv.get(), params) < 0
      && context != nullptr)
  {
    throw runtime_error("unable to create render context");
  }

  m_render = decltype(m_render) {context};
  mpv_render_context_set_update_callback(
      m_render.get(),
      [](void* ptr)
      {
        glfwPostEmptyEvent();
        auto& self = *reinterpret_cast<mpv_window*>(ptr);
        self.push_event(mpv_render_update_event {{self.weak_from_this()}});
      },
      this);

  mpv_node result {};
  const char* cmd[] = {"loadfile", path, nullptr};
  if (mpv_command_ret(m_mpv.get(), cmd, &result) < 0) {
    throw runtime_error("unable to open file using mpv");
  }

  const char* cmd2[] = {"set", "loop", "inf", nullptr};
  mpv_command_async(m_mpv.get(), 0, cmd2);

  if (mpv_observe_property(m_mpv.get(),
                           static_cast<int>(reply_data::video_out_params),
                           "video-out-params",
                           MPV_FORMAT_NODE)
      < 0)
  {
    throw runtime_error("unable to observe video-out-params properties");
  }

  mpv_set_wakeup_callback(
      m_mpv.get(), [](void*) { glfwPostEmptyEvent(); }, nullptr);
  mpv_window::handle_mpv_events();
}

auto mpv_window::try_show() -> void
{
  if (m_width == 0 || m_height == 0) {
    return;
  }

  glfwSetWindowSize(m_window_handle.get(), m_width, m_height);
  glfwSetWindowAspectRatio(m_window_handle.get(), m_width, m_height);
  glfwShowWindow(m_window_handle.get());
}

auto mpv_window::handle_mpv_events() -> void
{
  mpv_event* e = nullptr;
  while ((e = mpv_wait_event(m_mpv.get(), 0)) != nullptr
         && e->event_id != MPV_EVENT_NONE)
  {
    if (e->event_id == MPV_EVENT_PROPERTY_CHANGE) {
      auto& property = *reinterpret_cast<mpv_event_property*>(e->data);
      if (property.format == MPV_FORMAT_NONE) {
        continue;
      }
      switch (e->reply_userdata) {
        case static_cast<int>(reply_data::video_out_params):
          auto& node = *reinterpret_cast<mpv_node*>(property.data);
          assert(node.format == MPV_FORMAT_NODE_MAP);
          auto& list = *node.u.list;
          for (int i = 0; i < list.num; ++i) {
            if (std::strcmp("dw", list.keys[i]) == 0) {
              assert(list.values[i].format == MPV_FORMAT_INT64);
              m_width = static_cast<i32>(list.values[i].u.int64);
            } else if (std::strcmp("dh", list.keys[i]) == 0) {
              assert(list.values[i].format == MPV_FORMAT_INT64);
              m_height = static_cast<i32>(list.values[i].u.int64);
            }
          }
          try_show();
          break;
      }
    }
  }
}

auto mpv_window::handle_event(event& e) -> void
{
  auto cmd = [this](const char* name, auto&&... args)
  {
    const vector<string> cmd_string {fmt::format("{}", args)...};
    vector<const char*> cmd_args;

    cmd_args.push_back(name);
    // fmt::print("{} ", name);
    for (const auto& cmd_string_arg : cmd_string) {
      cmd_args.push_back(cmd_string_arg.c_str());
      // fmt::print("{} ", cmd_string_arg);
    }
    // fmt::print("\n");
    cmd_args.push_back(nullptr);

    if (mpv_command_async(m_mpv.get(), 0, cmd_args.data()) < 0) {
      fmt::print("unable to send {} command to mpv\n", name);
    }
  };
  visit(overloaded {[this](mpv_render_update_event&)
                    {
                      if (auto flags =
                              mpv_render_context_update(m_render.get()) > 0;
                          (flags & MPV_RENDER_UPDATE_FRAME) != 0)
                      {
                        m_redraw = true;
                      }
                    },
                    [&](play_pause_event& ev)
                    {
                      switch (ev.mode) {
                        case change_mode::set:
                          cmd("set", "pause", !ev.value);
                          break;
                        case change_mode::add_or_cycle:
                          if (ev.value) {
                            cmd("cycle", "pause");
                          }
                          break;
                      }
                    },
                    [&](speed_event& ev)
                    {
                      switch (ev.mode) {
                        case change_mode::set:
                          cmd("set", "speed", ev.value);
                          break;
                        case change_mode::add_or_cycle:
                          cmd("add", "speed", ev.value);
                          break;
                      }
                    },
                    [&](seek_event& ev)
                    {
                      switch (ev.mode) {
                        case change_mode::set:
                          cmd("seek", ev.value, "absolute");
                          break;
                        case change_mode::add_or_cycle:
                          cmd("seek", ev.value);
                          break;
                      }
                    },
                    [](auto&) {}},
        e);
}

auto mpv_window::render() -> double
{
  window::render();
  handle_mpv_events();
  if (!m_redraw) {
    // mpv_set_wakeup_callback will wake the loop up,
    // so one can use the default timeout (or even wait
    // forever)
    return window::render();
  }

  m_redraw = false;
  make_context_current();

  int width = 0, height = 0;
  glfwGetFramebufferSize(m_window_handle.get(), &width, &height);
  m_gl.Viewport(0, 0, width, height);

  mpv_opengl_fbo fbo {0, width, height, 0};
  int i_true = 1;
  mpv_render_param params[] = {{MPV_RENDER_PARAM_OPENGL_FBO, &fbo},
                               {MPV_RENDER_PARAM_FLIP_Y, &i_true},
                               {MPV_RENDER_PARAM_INVALID, nullptr}};
  mpv_render_context_render(m_render.get(), params);
  swap_buffers();
  // vsync
  return -1;
}

}  // namespace imgv
