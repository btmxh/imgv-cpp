#pragma once

#include <filesystem>
#include <string_view>
#include <variant>
#include <vector>

#include <fmt/core.h>

#include "types.hpp"
#include "window.hpp"

#define NFD_THROWS_EXCEPTIONS
#include <nfd.hpp>

namespace imgv
{

using nfd = NFD::Guard;

class root_window;
class context
{
public:
  context(const vector<const char*>& args, bool& would_run);
  auto run() -> void;

private:
  nfd m_nfd;
  shared_ptr<root_window> m_root_window;
  vector<shared_ptr<window>> m_windows;
  shared_event_queue m_queue;

  auto open(const char* path) -> void;
  auto create_window(const char* media_path) -> shared_ptr<window>;
};
}  // namespace imgv
