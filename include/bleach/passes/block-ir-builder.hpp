#pragma once

#include "bleach/lifter/instr-impl.hpp"
#include "mctomir/symbols.h"

#include <llvm/IR/PassManager.h>

namespace bleach::lifter {
using namespace llvm;

class block_ir_builder_pass : public PassInfoMixin<block_ir_builder_pass> {
  const instr_impl &instrs;
  std::string state_struct_file;
  const mctomir::file_info *finfo;
  unsigned stack_size;
  std::string lifted_prefix;
  bool assume_functions_nop;

public:
  block_ir_builder_pass(const instr_impl &insts, bool functions_nop,
                        std::string_view state_file,
                        const mctomir::file_info *file_info, unsigned stack_sz,
                        std::string_view prefix)
      : instrs(insts), state_struct_file(state_file), finfo(file_info),
        stack_size(stack_sz), lifted_prefix(prefix),
        assume_functions_nop(functions_nop) {}

  PreservedAnalyses run(Module &m, ModuleAnalysisManager &mam);
};

} // namespace bleach::lifter
