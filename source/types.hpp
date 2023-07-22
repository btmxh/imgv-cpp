#pragma once

#include <array>
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

#include <fmt/core.h>

namespace imgv
{
namespace fs = std::filesystem;

using fs::path;
using std::array;
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

using u8 = std::uint8_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using usize = std::size_t;

// NOLINTNEXTLINE(*-macro-*)
#define IMGV_ERROR(...) std::throw_with_nested(runtime_error(__VA_ARGS__))

inline auto dump_exception(const std::exception& e, usize level = 0) -> void
{
  fmt::print("{}{}\n", std::string(level, ' '), e.what());
  try {
    std::rethrow_if_nested(e);
  } catch (const std::exception& nested_ex) {
    dump_exception(nested_ex, level + 1);
  } catch (...) {
  }
}
}  // namespace imgv
