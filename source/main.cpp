#include <cstddef>

#include "context.hpp"
#include "types.hpp"

auto main(int argc, const char* argv[]) -> int
{
  using namespace imgv;
  vector<const char*> args;
  args.reserve(argc <= 0 ? 0 : static_cast<std::size_t>(argc - 1));
  for (int i = 1; i < argc; ++i) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    args.push_back(argv[i]);
  }

  bool run = true;
  context c {args, run};
  if (run) {
    c.run();
  }

  return 0;
}
