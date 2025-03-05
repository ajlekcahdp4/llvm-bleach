#pragma once

#include "bleach/lifter/instr-impl.h"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Support/Regex.h>

#include <map>
#include <unordered_set>

namespace llvm {
class MachineBasicBlock;
class BasicBlock;
} // namespace llvm

namespace bleach::lifter {
using namespace llvm;

class block_ir_builder_pass : public PassInfoMixin<block_ir_builder_pass> {
  const instr_impl &instrs;

public:
  block_ir_builder_pass(const instr_impl &insts) : instrs(insts) {}

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

class mbb2bb final
    : private std::unordered_map<const MachineBasicBlock *, BasicBlock *> {
public:
  using unordered_map::begin;
  using unordered_map::empty;
  using unordered_map::end;
  using unordered_map::erase;
  using unordered_map::insert;
  using unordered_map::size;
  auto operator[](const MachineBasicBlock *mbb) const {
    auto found = find(mbb);
    if (found == end()) {
      throw std::invalid_argument("Attempt to get Basic Block for unknown MBB");
    }
    return found->second;
  }
};

class register_class final : private std::unordered_set<unsigned> {
  llvm::Regex regex;
  const TargetRegisterClass *rclass = nullptr;
  unsigned reg_bitsize = 0;

public:
  using unordered_set::begin;
  using unordered_set::contains;
  using unordered_set::empty;
  using unordered_set::end;
  using unordered_set::insert;
  using unordered_set::size;

  register_class(llvm::Regex &&rx) : regex(std::move(rx)) {}

  void add_reg(unsigned reg) { insert(reg); }

  auto &get_regex() const { return regex; }

  auto *get_target_regclass() const { return rclass; }

  void set_rclass(const TargetRegisterClass *rcl, unsigned regsize) {
    rclass = rcl;
    reg_bitsize = regsize;
  }

  unsigned get_register_size() const { return reg_bitsize; }
};

class register_stats final : std::vector<register_class> {
public:
  using vector::begin;
  using vector::empty;
  using vector::end;
  using vector::size;
  using vector::operator[];

  template <typename It> register_stats(It start, It finish) {
    for (; start != finish; ++start)
      vector::emplace_back(Regex(*start));
  }

  auto &get_register_class_for(unsigned reg) const {
    auto found =
        find_if(*this, [reg](auto &rclass) { return rclass.contains(reg); });
    if (found == end()) {
      throw std::invalid_argument("Unknown register encountered");
    }
    return *found;
  }

  auto *get_target_register_class_for(unsigned reg) const {
    return get_register_class_for(reg).get_target_regclass();
  }
};

void fill_ir_for_bb(MachineBasicBlock &mbb, reg2vals &rmap,
                    const instr_impl &instrs, const LLVMTargetMachine &tm,
                    const mbb2bb &m2b, StructType &state,
                    const register_stats &reg_stats);

struct basic_block {
  MachineBasicBlock *mbb;
  BasicBlock *bb;
};

void copy_instructions(const MachineBasicBlock &src, MachineBasicBlock &dst);

basic_block clone_basic_block(MachineBasicBlock &src, MachineFunction &dst);

StructType &create_state_type(LLVMContext &ctx);

} // namespace bleach::lifter
