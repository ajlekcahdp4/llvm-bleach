#pragma once

#include "mctomir/target/target.hpp"

#include <string>

namespace mctomir {
using namespace llvm;

class riscv_target : public target {
public:
  auto get_name() const -> std::string override { return "riscv"; };

  auto is_return(MachineInstr &minst) const -> bool override;

  auto insert_return(MachineBasicBlock &mblock, MachineBasicBlock::iterator ins,
                     LLVMTargetMachine &tmachine) const -> bool override;
};

} // namespace mctomir
