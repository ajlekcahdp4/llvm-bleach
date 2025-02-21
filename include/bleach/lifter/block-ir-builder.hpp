#pragma once

#include "bleach/lifter/instr-impl.h"
#include "bleach/target/target.hpp"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/PassManager.h>

#include <map>

namespace llvm {
class MachineBasicBlock;
class BasicBlock;
} // namespace llvm

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

class reg2vals final : private std::map<unsigned, Value *> {
public:
  using map::at;
  using map::begin;
  using map::contains;
  using map::end;
  using map::try_emplace;
  using map::operator[];
  void dump() const {
    for (auto &[reg, val] : *this) {
      errs() << "REG: " << reg << " val: " << *val << "\n";
    }
  }
};

class mbb2bb final : private DenseMap<const MachineBasicBlock *, BasicBlock *> {
public:
  using DenseMap::begin;
  using DenseMap::empty;
  using DenseMap::end;
  using DenseMap::erase;
  using DenseMap::insert;
  using DenseMap::size;
  auto operator[](const MachineBasicBlock *mbb) const {
    auto found = find(mbb);
    if (found == end()) {
      throw std::invalid_argument("Attempt to get Basic Block for unknown MBB");
    }
    return found->second;
  }
};

void fill_ir_for_bb(MachineBasicBlock &mbb, reg2vals &rmap,
                    const instr_impl &instrs, const LLVMTargetMachine &tm,
                    const target &tgt, const mbb2bb &m2b, StructType &state);
struct basic_block {
  MachineBasicBlock *mbb;
  BasicBlock *bb;
};

void copy_instructions(const MachineBasicBlock &src, MachineBasicBlock &dst);

basic_block clone_basic_block(MachineBasicBlock &src, MachineFunction &dst);

StructType &create_state_type(LLVMContext &ctx);

} // namespace bleach::lifter
