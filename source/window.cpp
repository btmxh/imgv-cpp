#include <algorithm>
#include <cmath>
#include <fstream>

#include "window.hpp"

#include <fmt/core.h>

#include "root_window.hpp"

#ifdef IMGV_X11
#  define GLFW_EXPOSE_NATIVE_X11
#  include <GLFW/glfw3native.h>
#  include <X11/Xatom.h>
#  include <X11/Xlib.h>
#endif

#include "animated_image_window.hpp"
#include "gif.hpp"
#include "mpv_window.hpp"
#include "static_image_window.hpp"
#include "stbi.hpp"
#include "webp.hpp"

namespace imgv
{

auto glfw_window_deleter::operator()(GLFWwindow* window) noexcept -> void
{
  glfwDestroyWindow(window);
}

static auto set_x11_window_mode(GLFWwindow* window) -> void
{
#ifdef IMGV_X11
  auto* const x11_display = glfwGetX11Display();
  const auto x11_window = glfwGetX11Window(window);
  const auto window_type = XInternAtom(x11_display, "_NET_WM_WINDOW_TYPE", 0);
  const auto splash = XInternAtom(x11_display, "_NET_WM_WINDOW_TYPE_SPLASH", 0);
  XChangeProperty(x11_display,
                  x11_window,
                  window_type,
                  XA_ATOM,
                  32,
                  PropModeReplace,
                  reinterpret_cast<_Xconst unsigned char*>(&splash),
                  1);
#endif
}

window::window(weak_event_queue queue)
    : m_root(root_window::get())
    , m_queue {move(queue)}
{
  glfwDefaultWindowHints();
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
  glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
  glfwWindowHintString(GLFW_X11_CLASS_NAME, "imgv");
  glfwWindowHintString(GLFW_X11_INSTANCE_NAME, "imgv");
  m_window_handle = glfw_window {glfwCreateWindow(default_size,
                                                  default_size,
                                                  "imgv temp title",
                                                  nullptr,
                                                  m_root->get_glfw_handle())};
  set_x11_window_mode(m_window_handle.get());
  if (!m_window_handle) {
    IMGV_ERROR("unable to create window");
  }

  make_context_current();
  glfwSwapInterval(1);
  if (!gladLoadGLContext(&m_gl, glfwGetProcAddress)) {
    IMGV_ERROR("unable to load OpenGL function pointers");
  }

  glfwSetWindowUserPointer(m_window_handle.get(), this);
  glfwSetWindowRefreshCallback(
      m_window_handle.get(),
      [](GLFWwindow* w) {
        reinterpret_cast<window*>(glfwGetWindowUserPointer(w))->m_redraw = true;
      });
  glfwSetWindowCloseCallback(
      m_window_handle.get(),
      [](GLFWwindow* w)
      {
        reinterpret_cast<window*>(glfwGetWindowUserPointer(w))->m_dead = true;
        glfwHideWindow(w);
      });
  glfwSetKeyCallback(
      m_window_handle.get(),
      [](GLFWwindow* w, int key, int, int action, int)
      {
        auto& self = *reinterpret_cast<window*>(glfwGetWindowUserPointer(w));
        if (action == GLFW_RELEASE) {
          return;
        }
        if (key == GLFW_KEY_RIGHT) {
          self.push_event(imgv::seek_event {{self.weak_from_this()},
                                            {change_mode::add_or_cycle, 10.0}});
        } else if (key == GLFW_KEY_LEFT) {
          self.push_event(imgv::seek_event {
              {self.weak_from_this()}, {change_mode::add_or_cycle, -10.0}});
        } else if (key == GLFW_KEY_LEFT_BRACKET) {
          self.push_event(imgv::speed_event {
              {self.weak_from_this()}, {change_mode::add_or_cycle, -0.1}});
        } else if (key == GLFW_KEY_RIGHT_BRACKET) {
          self.push_event(imgv::speed_event {{self.weak_from_this()},
                                             {change_mode::add_or_cycle, 0.1}});
        } else if (key == GLFW_KEY_SPACE) {
          self.push_event(imgv::play_pause_event {
              {self.weak_from_this()}, {change_mode::add_or_cycle, true}});
        }
      });
  glfwSetMouseButtonCallback(
      m_window_handle.get(),
      [](GLFWwindow* w, int button, int action, int)
      {
        auto& self = *reinterpret_cast<window*>(glfwGetWindowUserPointer(w));
        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
          self.push_event(play_pause_event {
              {self.weak_from_this()},
              {change_mode::add_or_cycle, true},
          });
        } else if (button == GLFW_MOUSE_BUTTON_LEFT) {
          if (action == GLFW_PRESS) {
            self.m_drag_state.holding = true;
            std::tie(self.m_drag_state.ox, self.m_drag_state.oy) =
                window_drag_state::get_monitor_cursor_pos(w);
          } else if (action == GLFW_RELEASE) {
            self.m_drag_state.holding = false;
            self.m_drag_state.ox = 0;
            self.m_drag_state.oy = 0;
          }
        }
      });
  glfwSetCursorPosCallback(
      m_window_handle.get(),
      [](GLFWwindow* w, double, double)
      {
        auto& self = *reinterpret_cast<window*>(glfwGetWindowUserPointer(w));
        if (self.m_drag_state.holding) {
          auto&& [x, y] = window_drag_state::get_monitor_cursor_pos(w);
          self.m_drag_state.dx = x - self.m_drag_state.ox;
          self.m_drag_state.dy = y - self.m_drag_state.oy;
        }
      });
  glfwSetScrollCallback(
      m_window_handle.get(),
      [](GLFWwindow* wnd, double, double sy)
      {
        static constexpr int increment = 30;
        auto dw = static_cast<int>(sy * increment);
        int w = 0, h = 0;
        glfwGetWindowSize(wnd, &w, &h);
        const int nw = w + dw;
        const int nh = nw * h / w;
        auto&& [x, y] = window_drag_state::get_window_pos(wnd);
        glfwSetWindowSize(wnd, nw, nh);
        glfwSetWindowPos(wnd, x - (nw - w) / 2, y - (nh - h) / 2);
      });
}

auto window::push_event(event e) -> void
{
  if (auto queue = m_queue.lock(); queue != nullptr) {
    queue->push(move(e));
  }
}

auto window::dead() const -> bool
{
  return m_dead;
}

auto window::make_context_current() const -> void
{
  glfwMakeContextCurrent(m_window_handle.get());
}

auto window::make_context_non_current() -> void
{
  glfwMakeContextCurrent(nullptr);
}

auto window::swap_buffers() const -> void
{
  glfwSwapBuffers(m_window_handle.get());
}

auto window::show_window(int width, int height, const char* title) -> void
{
  glfwSetWindowSize(m_window_handle.get(), width, height);
  glfwSetWindowAspectRatio(m_window_handle.get(), width, height);
  glfwSetWindowTitle(m_window_handle.get(), title);
  glfwShowWindow(m_window_handle.get());
}

auto window_drag_state::update(GLFWwindow* window) -> void
{
  if (holding) {
    int x = 0, y = 0;
    glfwGetWindowPos(window, &x, &y);
    glfwSetWindowPos(window,
                     x + static_cast<int>(std::floor(dx)),
                     y + static_cast<int>(std::floor(dy)));
    ox += dx;
    oy += dy;
    dx = dy = 0;
  }
}

auto window_drag_state::get_window_pos(GLFWwindow* window) -> tuple<int, int>
{
  tuple<int, int> ret;
  glfwGetWindowPos(window, &std::get<0>(ret), &std::get<1>(ret));
  return ret;
}

auto window_drag_state::get_cursor_pos(GLFWwindow* window)
    -> tuple<double, double>
{
  tuple<double, double> ret;
  glfwGetCursorPos(window, &std::get<0>(ret), &std::get<1>(ret));
  return ret;
}

auto window_drag_state::get_monitor_cursor_pos(GLFWwindow* window)
    -> tuple<double, double>
{
  auto&& [wx, wy] = get_window_pos(window);
  auto&& [cx, cy] = get_cursor_pos(window);
  return std::make_tuple(wx + cx, wy + cy);
}

using namespace std::literals;
struct path_checker
{
  const char* path;
  std::ifstream file {};

  constexpr static usize max_header_size = 16;
  usize header_size = 0;
  array<char, max_header_size> header {};

  auto check_file_and_open() -> bool
  {
    file.open(path);
    return !file.bad();
  }
  auto read_header() -> void
  {
    if (file.tellg() != 0) {
      return;
    }

    header_size = static_cast<usize>(std::max<std::streamsize>(
        file.readsome(header.data(), max_header_size), 0));
  }

  auto check_header(string_view check, usize offset = 0) -> bool
  {
    read_header();
    return memcmp(check.data(), &header.at(offset), check.size()) == 0;
  }

  auto is_png() -> bool
  {
    return check_header("\x89\x50\x4E\x47\x0D\x0A\x1A\x0A"sv);
  }

  auto is_gif() -> bool
  {
    return check_header("GIF87a"sv) || check_header("GIF89a"sv);
  }

  auto is_jpeg() -> bool
  {
    return check_header("\xFF\xD8\xFF\xDB"sv)
        || check_header("\xFF\xD8\xFF\xE0\x00\x10\x4A\x46\x49\x46\x00\x01"sv)
        || check_header("\xFF\xD8\xFF\xEE"sv)
        || (check_header("\xFF\xD8\xFF\xE1"sv)
            && check_header("\x45\x78\x69\x66\x00\x00"sv, 6))
        || check_header("\xFF\xD8\xFF\xE0"sv);
  }

  auto is_bmp() -> bool { return check_header("BM"); }
  auto is_psd() -> bool { return check_header("8BPS"); }
  auto is_hdr() -> bool { return check_header("#?RADIANCE."); }

  auto is_ppm() -> bool
  {
    return check_header("\x50\x33\x0A"sv) || check_header("\x50\x36\x0A"sv);
  }

  auto stbi_supported() -> bool
  {
    return is_png() || is_gif() || is_jpeg() || is_bmp() || is_psd() || is_hdr()
        || is_ppm();
  }

  auto is_webp() -> bool
  {
    return check_header("RIFF"sv) && check_header("WEBP"sv, 8);
  }
};

template<typename Loader>
inline auto open_loader(weak_event_queue queue, Loader loader)
    -> shared_ptr<window>
{
  if (loader.metadata.animated) {
    return std::make_shared<animated_image_window>(move(queue), move(loader));
  }

  return std::make_shared<static_image_window>(move(queue), loader);
}

auto create_window(weak_event_queue queue, const char* path)
    -> shared_ptr<window>
{
  using namespace std::placeholders;
  path_checker checker {path};
  if (checker.check_file_and_open()) {
    if (checker.is_gif()) {
      try {
        fmt::print("opening file using gif_loader\n");
        return open_loader(move(queue), gif_loader {path});
      } catch (std::exception& ex) {
        fmt::print("warn: unable to load gif file using gif_loader\n");
        dump_exception(ex);
      }
    }

    if (checker.is_webp()) {
      try {
        fmt::print("opening file using webp_loader\n");
        return open_loader(move(queue), webp_loader {path});
      } catch (std::exception& ex) {
        fmt::print("warn: unable to load gif file using webp_loader\n");
        dump_exception(ex);
      }
    }

    if (checker.stbi_supported()) {
      try {
        fmt::print("opening file using stbi_loader\n");
        return open_loader(move(queue), stbi_loader {path});
      } catch (std::exception& ex) {
        fmt::print("warn: unable to load gif file using stbi_loader\n");
        dump_exception(ex);
      }
    }
  }

  try {
    fmt::print("opening file using mpv_window\n");
    return std::make_shared<mpv_window>(move(queue), path);
  } catch (std::exception& ex) {
    fmt::print("warn: unable to display media file using mpv_window\n");
    dump_exception(ex);
  }

  IMGV_ERROR("unable to open media file");
}

}  // namespace imgv
