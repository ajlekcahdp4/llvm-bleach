#pragma once

#include <llvm/CodeGen/MachineBasicBlock.h>
#include <llvm/Target/TargetMachine.h>

#include <string>

namespace llvm {
class MachineInstr;
class BasicBlock;
class LLVMContext;
class MachineInstr;
class Value;
} // namespace llvm

namespace mctomir {
using namespace llvm;

class target {
public:
  virtual ~target() = default;

  virtual auto get_name() const -> std::string = 0;

  virtual auto is_return(MachineInstr &minst) const -> bool = 0;
  virtual auto insert_return(MachineBasicBlock &mblock,
                             MachineBasicBlock::iterator ins,
                             LLVMTargetMachine &tmachine) const -> bool = 0;
};

} // namespace mctomir
