#pragma once

#include <llvm/CodeGen/MachineFunction.h>
#include <llvm/IR/Instruction.h>

#include <unordered_map>
#include <vector>

namespace bleach::lifter {
using namespace llvm;

// @class instr_impl
// @brief map from instr opcode to llvm ir impl
class instr_impl final
    : private std::unordered_map<unsigned, MachineFunction *> {
public:
  instr_impl() = default;
  using unordered_map::at;
  using unordered_map::begin;
  using unordered_map::contains;
  using unordered_map::end;
  using unordered_map::find;
  using unordered_map::try_emplace;
};

} // namespace bleach::lifter
