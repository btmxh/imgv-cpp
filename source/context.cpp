#include <algorithm>

#include "context.hpp"

#include <boxer/boxer.h>
#include <fmt/core.h>
#include <nfd.hpp>

#include "gif_window.hpp"
#include "mpv_window.hpp"
#include "root_window.hpp"

namespace imgv
{
context::context(const vector<const char*>& args, bool& would_run)
    : m_root_window {root_window::get()}
    , m_queue {std::make_shared<event_queue>()}
{
  if (std::any_of(args.begin(),
                  args.end(),
                  [](const char* arg) {
                    return std::strcmp("-h", arg) == 0
                        || std::strcmp("--help", arg) == 0;
                  }))
  {
    fmt::print(
        "Usage: imgv [paths to media files]...\n"
        "If no arguments, the program will automatically launch a file dialog"
        " for the user to choose the media files.\n");
    would_run = false;
    return;
  }

  would_run = true;

  for (const auto& arg : args) {
    open(arg);
  }

  if (args.empty()) {
    NFD::UniquePathSet out_paths;
    NFD::UniquePathSetPathU8 out_path;
    switch (NFD::OpenDialogMultiple(out_paths)) {
      case NFD_ERROR:
        throw runtime_error("unable to open file chooser");
      case NFD_CANCEL:
        would_run = false;
        return;
      default:;
    }
    nfdpathsetsize_t count = 0;
    if (NFD::PathSet::Count(out_paths, count) == NFD_ERROR) {
      throw runtime_error("unable to get pathset count");
    }
    for (nfdpathsetsize_t i = 0; i < count; ++i) {
      if (NFD::PathSet::GetPath(out_paths, i, out_path) == NFD_ERROR) {
        fmt::print(stderr, "unable to get path of index {}\n", i);
        continue;
      }

      open(out_path.get());
    }
  }
}

auto context::open(const char* path) -> void
{
  try {
    m_windows.push_back(create_window(path));
  } catch (exception& ex) {
    const auto msg = fmt::format("error opening media file '{}'", path);
    boxer::show(
        ex.what(), msg.c_str(), boxer::Style::Error, boxer::Buttons::OK);
  }
}

auto context::run() -> void
{
  auto last_time = glfwGetTime();
  while (!m_windows.empty()) {
    // fmt::print("{}\n", (glfwGetTime() - last_time) * 1e3);
    last_time = glfwGetTime();

    m_windows.erase(std::remove_if(m_windows.begin(),
                                   m_windows.end(),
                                   [](const auto& w) { return w->dead(); }),
                    m_windows.end());

    for (auto e = m_queue->pop(); e.has_value(); e = m_queue->pop()) {
      if (auto w = handler(*e); w.has_value()) {
        if (w.value()) {
          w->get()->handle_event(*e);
        }
      } else {
        for (auto& win : m_windows) {
          win->handle_event(*e);
        }
      }
    }

    // 10 seconds at most
    auto wait_time = 10.0;
    for (auto& window : m_windows) {
      auto window_wait_time = window->render();
      wait_time = std::min(window_wait_time, wait_time);
    }

    if (wait_time < 0) {
      glfwPollEvents();
    } else {
      glfwWaitEventsTimeout(wait_time);
    }
  }
}

auto context::create_window(const char* media_path) -> shared_ptr<window>
{
  const string_view path {media_path};
  if (path.rfind(".gif") + 4 == path.size()) {
    return std::make_shared<gif_window>(m_queue, media_path);
  };

  return std::make_shared<mpv_window>(m_queue, media_path);
}

}  // namespace imgv
