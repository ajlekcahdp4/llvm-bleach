#pragma once

#include "bleach/lifter/instr-impl.hpp"

#include <llvm/IR/PassManager.h>

namespace bleach::lifter {
using namespace llvm;

class block_ir_builder_pass : public PassInfoMixin<block_ir_builder_pass> {
  const instr_impl &instrs;
  std::string state_struct_file;
  unsigned stack_size;
  bool assume_functions_nop;

public:
  block_ir_builder_pass(const instr_impl &insts, bool functions_nop,
                        std::string_view state_file, unsigned stack_sz)
      : instrs(insts), state_struct_file(state_file), stack_size(stack_sz),
        assume_functions_nop(functions_nop) {}

  PreservedAnalyses run(Module &m, ModuleAnalysisManager &mam);
};

} // namespace bleach::lifter
