#pragma once

#include <llvm/TargetParser/Triple.h>
namespace llvm {
class MachineInstr;
class BasicBlock;
class LLVMContext;
class Value;
} // namespace llvm

namespace bleach {
using namespace llvm;

class target {
public:
  virtual ~target() = default;

  virtual auto get_name() const -> std::string = 0;
};

} // namespace bleach
