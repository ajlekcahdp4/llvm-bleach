#include "bleach/lifter/block-ir-builder.hpp"

#include <llvm/CodeGen/MachineFunction.h>
#include <llvm/CodeGen/MachineModuleInfo.h>
#include <llvm/CodeGen/MachineRegisterInfo.h>
#include <llvm/CodeGen/TargetInstrInfo.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/Linker/Linker.h>
#include <llvm/MC/MCInstrInfo.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/Utils/Cloning.h>

#include <ranges>
#include <set>

namespace bleach::lifter {
namespace ranges = std::ranges;
using namespace llvm;

struct basic_block {
  MachineBasicBlock *mbb;
  BasicBlock *bb;
};

void copy_instructions(const MachineBasicBlock &src, MachineBasicBlock &dst) {
  auto *mf = dst.getParent();
  assert(mf);
  auto *instr_info = mf->getSubtarget().getInstrInfo();
  assert(instr_info);
  for (auto &instr : src.instrs() | ranges::views::filter([](auto &instr) {
                       return !instr.isBundledWithPred();
                     })) {
    auto &instr_copy = instr_info->duplicate(dst, dst.end(), instr);
    if (instr_copy.isBranch())
      for (auto &op : instr_copy.operands())
        if (op.isMBB() && op.getMBB() == &src)
          op.setMBB(&dst);
  }
}

basic_block clone_basic_block(MachineBasicBlock &src, BasicBlock &dummy) {
  auto *mf = src.getParent();
  assert(mf);
  auto &func = mf->getFunction();
  auto *new_block = BasicBlock::Create(func.getContext(), "", &func);
  assert(new_block);
  // FIXME: remove this
  IRBuilder builder(func.getContext());
  builder.SetInsertPoint(new_block);
  builder.CreateBr(&dummy);

  auto *new_mblock = mf->CreateMachineBasicBlock(new_block);
  assert(new_mblock);
  mf->push_back(new_mblock);
  copy_instructions(src, *new_mblock);

  for (auto it = src.succ_begin(); it != src.succ_end(); ++it)
    new_mblock->copySuccessor(&src, it);

  return {new_mblock, new_block};
}

void create_basic_blocks_for_mfunc(MachineFunction &mfunc, mbb2bb &m2b) {
  std::vector<MachineBasicBlock *> mblocks_to_erase;
  std::vector<BasicBlock *> blocks_to_erase;
  transform(mfunc, std::back_inserter(mblocks_to_erase),
            [](auto &mbb) { return &mbb; });
  auto &func = mfunc.getFunction();
  transform(func, std::back_inserter(blocks_to_erase),
            [](auto &bb) { return &bb; });
  // Dummy basic block is necessary due to bug in
  // MachineFunction::CreateMachineBasicBlock which dereferences nullptr if BB
  // does not contain terminator.
  // FIXME: remove after fix in upstream
  auto *dummy_bb = BasicBlock::Create(func.getContext(), "dummy", &func);
  std::unordered_map<MachineBasicBlock *, MachineBasicBlock *> block_map;
  // One cannot simply loop over mfunc as we insert new blocks into it
  for (auto *mbb : mblocks_to_erase) {
    auto [new_mblock, new_block] = clone_basic_block(*mbb, *dummy_bb);
    m2b.insert({new_mblock, new_block});
    block_map.insert({mbb, new_mblock});
  }
  for (auto [old_block, new_block] : block_map) {
    for (auto &mbb : mfunc) {
      if (&mbb == old_block)
        continue;
      if (!is_contained(mbb.successors(), old_block))
        continue;
      mbb.ReplaceUsesOfBlockWith(old_block, new_block);
    }
  }
  for (auto *mbb : mblocks_to_erase) {
    m2b.erase(mbb);
    mbb->eraseFromParent();
  }
  for (auto *bb : blocks_to_erase)
    bb->eraseFromParent();
  for (auto &bb : func)
    for (auto &instr : make_early_inc_range(bb))
      if (auto *branch = dyn_cast<BranchInst>(&instr))
        if (is_contained(branch->successors(), dummy_bb))
          instr.eraseFromParent();
}

void fill_module_with_instrs(Module &m, const instr_impl &instrs) {
  assert(!instrs.empty());
  auto first = CloneModule(*instrs.begin()->ir_module);
  for (auto &&i : drop_begin(instrs)) {
    assert(i.ir_module);
    auto module_clone = CloneModule(*i.ir_module);
    Linker::linkModules(*first, std::move(module_clone));
  }
  Linker::linkModules(m, std::move(first));
}

auto *generate_function_object(Module &m, MachineFunction &mf, reg2vals &rmap) {
  auto *ret_type = Type::getIntNTy(m.getContext(), 64);
  auto &reg_info = mf.getRegInfo();
  std::vector<Type *> args;
  for ([[maybe_unused]] auto &&_ : reg_info.liveins())
    args.push_back(Type::getIntNTy(m.getContext(), 64));
  auto *func_type = FunctionType::get(ret_type, args, /* is var arg */ false);
  auto *func =
      Function::Create(func_type, Function::ExternalLinkage, mf.getName(), m);
  for (auto &&[livein, arg] : zip(reg_info.liveins(), func->args()))
    rmap.try_emplace(livein.first, &arg);
  return func;
}

auto generate_function(Module &m, MachineFunction &mf, const instr_impl &instrs,
                       const LLVMTargetMachine &target_machine,
                       const target &tgt, const mbb2bb &m2b) {
  reg2vals rmap;
  auto *func = generate_function_object(m, mf, rmap);
  auto &f = mf.getFunction();
  for (auto &bb : make_early_inc_range(f)) {
    auto known_blocks = make_second_range(m2b);
    bb.erase(bb.begin(), bb.end());
    if (!is_contained(known_blocks, &bb))
      bb.eraseFromParent();
  }
  for (auto &mbb : mf)
    fill_ir_for_bb(mbb, *func, rmap, instrs, target_machine, tgt, m2b);
}

std::string get_instruction_name(const MachineInstr &minst,
                                 const MCInstrInfo &instr_info) {
  return instr_info.getName(minst.getOpcode()).str();
}

void create_basic_blocks(Module &m, MachineModuleInfo &mmi, mbb2bb &m2b,
                         const std::set<std::string> &target_functions) {
  for (auto &f : m | ranges::views::filter([&target_functions](auto &f) {
                   return target_functions.contains(f.getName().str());
                 })) {
    auto &mf = mmi.getOrCreateMachineFunction(f);
    create_basic_blocks_for_mfunc(mf, m2b);
  }
}

PreservedAnalyses block_ir_builder_pass::run(Module &m,
                                             ModuleAnalysisManager &mam) {
  std::set<std::string> target_functions;
  transform(m, std::inserter(target_functions, target_functions.end()),
            [](auto &f) { return f.getName().str(); });
  fill_module_with_instrs(m, instrs);
  auto m2b = mbb2bb{};
  auto &mmi = mam.getResult<MachineModuleAnalysis>(m).getMMI();
  create_basic_blocks(m, mmi, m2b, target_functions);
  for (auto &f : m | ranges::views::filter([&target_functions](auto &f) {
                   return target_functions.contains(f.getName().str());
                 })) {
    auto &mf = mmi.getOrCreateMachineFunction(f);
    generate_function(m, mf, instrs, mmi.getTarget(), tgt, m2b);
  }
  return PreservedAnalyses::none();
}

auto generate_jump(const MachineInstr &minst, BasicBlock &bb, reg2vals &rmap,
                   const instr_impl &instrs, LLVMContext &ctx,
                   const LLVMTargetMachine &target, const mbb2bb &m2b) {
  assert(minst.isBranch());
  auto mbb_op =
      llvm::find_if(minst.operands(), [](auto &op) { return op.isMBB(); });
  assert(mbb_op != minst.operands_end());
  auto *mbb = mbb_op->getMBB();
  assert(mbb);
  auto *target_bb = m2b[mbb];
  assert(target_bb);
  IRBuilder builder(ctx);
  builder.SetInsertPoint(&bb);
  return builder.CreateBr(target_bb);
}

auto get_if_true_block(const MachineInstr &minst,
                       const mbb2bb &m2b) -> BasicBlock * {
  assert(minst.isConditionalBranch());
  auto &op = minst.getOperand(2);
  assert(op.isMBB());
  return m2b[op.getMBB()];
}

auto get_if_false_block(const MachineInstr &minst,
                        const mbb2bb &m2b) -> BasicBlock * {
  assert(minst.isConditionalBranch());
  assert(minst.getNumOperands() >= 2);
  auto next = std::next(minst.getIterator());
  assert(next != minst.getParent()->end());
  assert(next->isUnconditionalBranch());
  auto &op = next->getOperand(0);
  assert(op.isMBB());
  return m2b[op.getMBB()];
}

auto generate_branch(const MachineInstr &minst, BasicBlock &bb, reg2vals &rmap,
                     const instr_impl &instrs, LLVMContext &ctx,
                     const LLVMTargetMachine &target_machine, const target &tgt,
                     const mbb2bb &m2b) -> Instruction * {
  auto *cond =
      tgt.create_branch_condition(minst, bb, {rmap.begin(), rmap.end()}, ctx);
  IRBuilder builder(ctx);
  builder.SetInsertPoint(&bb);
  auto *if_true = get_if_true_block(minst, m2b);
  auto *if_false = get_if_false_block(minst, m2b);
  return builder.CreateCondBr(cond, if_true, if_false);
}

auto generate_instruction(const MachineInstr &minst, BasicBlock &bb,
                          reg2vals &rmap, const instr_impl &instrs,
                          LLVMContext &ctx,
                          const LLVMTargetMachine &target_machine,
                          const target &tgt, const mbb2bb &m2b) -> Value * {
  auto *iinfo = target_machine.getMCInstrInfo();
  auto name = get_instruction_name(minst, *iinfo);
  IRBuilder builder(ctx);
  builder.SetInsertPoint(&bb);
  if (minst.isBranch()) {
    if (minst.isUnconditionalBranch())
      return generate_jump(minst, bb, rmap, instrs, ctx, target_machine, m2b);
    assert(minst.isConditionalBranch());
    return generate_branch(minst, bb, rmap, instrs, ctx, target_machine, tgt,
                           m2b);
  }
  auto &instr_module = instrs.get(name);
  auto *func = instr_module.getFunction(name);
  if (!func)
    throw std::runtime_error("Could not find \"" + name + "\" in module");
  std::vector<Value *> args;
  for (auto &&use : minst.uses()) {
    assert(use.isUse());
    if (!use.isReg())
      throw std::runtime_error("Only register operands are supported");
    assert(rmap.contains(use.getReg().id()));
    args.push_back(rmap.at(use.getReg().id()));
  }
  auto *call = builder.CreateCall(func->getFunctionType(), func, args);
  for (auto &def : minst.defs()) {
    assert(def.isDef());
    if (!def.isReg())
      throw std::runtime_error("Only register operands are supported");
    auto reg = def.getReg();
    rmap[reg.id()] = call;
  }
  return call;
}

void fill_ir_for_bb(MachineBasicBlock &mbb, Function &func, reg2vals &rmap,
                    const instr_impl &instrs,
                    const LLVMTargetMachine &target_machine, const target &tgt,
                    const mbb2bb &m2b) {
  auto *bb = m2b[&mbb];
  assert(bb);
  for (auto &minst : mbb) {
    generate_instruction(minst, *bb, rmap, instrs, bb->getContext(),
                         target_machine, tgt, m2b);
  }
}

} // namespace bleach::lifter
