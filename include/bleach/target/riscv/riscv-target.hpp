#pragma once

#include "bleach/target/target.hpp"

#include <string>

namespace bleach {

class riscv_target : public target {
public:
  auto get_name() const -> std::string override { return "riscv"; };

  auto
  create_branch_condition(const MachineInstr &minst, BasicBlock &bb,
                          const std::unordered_map<unsigned, Value *> &rmap,
                          LLVMContext &ctx) const -> Value * override;
};

} // namespace bleach
