#include "bleach/target/target-selector.hpp"

#include "bleach/target/riscv/riscv-target.hpp"

namespace bleach {

auto select_target(const Triple &triple) -> std::unique_ptr<target> {
  if (auto &&arch = triple.getArch();
      arch == Triple::riscv32 || arch == Triple::riscv64) {
    return std::make_unique<riscv_target>();
  }
  throw std::invalid_argument("Unknown target: \"" + triple.normalize() + "\"");
}

} // namespace bleach
