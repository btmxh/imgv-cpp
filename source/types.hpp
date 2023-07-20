#pragma once

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace imgv
{
namespace fs = std::filesystem;

using fs::path;
using std::exception;
using std::forward;
using std::move;
using std::mutex;
using std::nullopt;
using std::optional;
using std::runtime_error;
using std::scoped_lock;
using std::shared_ptr;
using std::string;
using std::string_view;
using std::tuple;
using std::unique_ptr;
using std::variant;
using std::vector;
using std::visit;
using std::weak_ptr;

template<class... Ts>
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
struct overloaded : Ts...
{
  using Ts::operator()...;
};
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

using i32 = std::int32_t;
using i64 = std::int64_t;
using usize = std::size_t;
}  // namespace imgv
