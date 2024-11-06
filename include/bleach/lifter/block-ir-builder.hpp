#pragma once

#include "bleach/lifter/instr-impl.h"
#include "bleach/target/target.hpp"

#include <llvm/IR/PassManager.h>

namespace llvm {
class MachineBasicBlock;
}

namespace bleach::lifter {
using namespace llvm;

class block_ir_builder_pass : public PassInfoMixin<block_ir_builder_pass> {
  const instr_impl &instrs;
  const target &tgt;

public:
  block_ir_builder_pass(const instr_impl &insts, const target &target)
      : instrs(insts), tgt(target) {}

  PreservedAnalyses run(Module &m, ModuleAnalysisManager &mam);
};

class reg2vals final : private std::unordered_map<unsigned, Value *> {
public:
  using unordered_map::at;
  using unordered_map::begin;
  using unordered_map::end;
  using unordered_map::try_emplace;
  using unordered_map::operator[];
};

void fill_ir_for_bb(MachineBasicBlock &mbb, Function &func, reg2vals &rmap,
                    const instr_impl &instrs, const LLVMTargetMachine &tm,
                    const target &tgt);

void fill_module_with_instrs(Module &m, const instr_impl &instrs);

} // namespace bleach::lifter
