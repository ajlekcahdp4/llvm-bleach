#pragma once

#include "bleach/target/target.hpp"

#include <string>

namespace bleach {

class riscv_target : public target {
public:
  auto get_name() const -> std::string override { return "riscv"; };
};

} // namespace bleach
