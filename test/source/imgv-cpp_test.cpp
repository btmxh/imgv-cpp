#include "lib.hpp"

auto main() -> int
{
  auto const lib = library {};

  return lib.name == "imgv-cpp" ? 0 : 1;
}
