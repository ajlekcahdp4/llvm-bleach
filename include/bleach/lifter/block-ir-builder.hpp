#pragma once

#include "bleach/lifter/instr-impl.h"

#include <llvm/IR/PassManager.h>

namespace llvm {
class MachineBasicBlock;
}

namespace bleach::lifter {
using namespace llvm;

class block_ir_builder_pass : public PassInfoMixin<block_ir_builder_pass> {
  const instr_impl &instrs;

public:
  block_ir_builder_pass(const instr_impl &insts) : instrs(insts) {}

  PreservedAnalyses run(Module &m, ModuleAnalysisManager &mam);
};

class reg2vals final : private std::unordered_map<unsigned, Value *> {
public:
  using unordered_map::at;
  using unordered_map::begin;
  using unordered_map::end;
  using unordered_map::try_emplace;
};

void fill_ir_for_bb(MachineBasicBlock &mbb, Function &func, reg2vals &rmap,
                    const instr_impl &instrs, const LLVMTargetMachine &tm);

void fill_module_with_instrs(Module &m, const instr_impl &instrs);

} // namespace bleach::lifter
