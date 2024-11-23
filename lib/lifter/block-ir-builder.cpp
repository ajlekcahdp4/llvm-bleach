#include "bleach/lifter/block-ir-builder.hpp"

#include <llvm/CodeGen/MachineFunction.h>
#include <llvm/CodeGen/MachineModuleInfo.h>
#include <llvm/CodeGen/MachineRegisterInfo.h>
#include <llvm/CodeGen/TargetInstrInfo.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
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

basic_block clone_basic_block(MachineBasicBlock &src, MachineFunction &dst) {
  auto &func = dst.getFunction();
  auto *new_block = BasicBlock::Create(func.getContext(), "", &func);
  assert(new_block);
  // Dummy basic block is necessary due to bug in
  // MachineFunction::CreateMachineBasicBlock which dereferences nullptr if BB
  // does not contain terminator.
  // FIXME: remove after fix in upstream
  auto *dummy_bb = BasicBlock::Create(func.getContext(), "dummy", &func);
  IRBuilder builder(func.getContext());
  builder.SetInsertPoint(new_block);
  builder.CreateBr(dummy_bb);

  auto *new_mblock = dst.CreateMachineBasicBlock(new_block);
  assert(new_mblock);
  dst.push_back(new_mblock);
  copy_instructions(src, *new_mblock);

  for (auto it = src.succ_begin(); it != src.succ_end(); ++it)
    new_mblock->copySuccessor(&src, it);
  // remove dummy instruction
  assert(!new_block->empty());
  new_block->begin()->eraseFromParent();
  dummy_bb->eraseFromParent();
  return {new_mblock, new_block};
}

void create_basic_blocks_for_mfunc(MachineFunction &src, MachineFunction &dst,
                                   mbb2bb &m2b) {
  std::unordered_map<MachineBasicBlock *, MachineBasicBlock *> block_map;
  // One cannot simply loop over mfunc as we insert new blocks into it
  for (auto &mbb : src) {
    auto [new_mblock, new_block] = clone_basic_block(mbb, dst);
    m2b.insert({new_mblock, new_block});
    block_map.insert({&mbb, new_mblock});
  }
  for (auto [old_block, new_block] : block_map) {
    for (auto &mbb : dst) {
      if (&mbb == old_block)
        continue;
      if (!is_contained(mbb.successors(), old_block))
        continue;
      mbb.ReplaceUsesOfBlockWith(old_block, new_block);
    }
  }
  for (auto &mbb : src)
    m2b.erase(&mbb);
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

void materialize_registers(MachineFunction &mf, Function &func, reg2vals &rmap,
                           const LLVMTargetMachine &target_machine,
                           StructType &state) {
  if (mf.empty())
    return;
  auto &ctx = func.getContext();
  constexpr auto gpr_array_idx = 0u;
  Value *state_arg = func.getArg(gpr_array_idx);
  auto *array_type = *state.element_begin();
  assert(!func.empty());
  auto &first_block = func.front();
  auto builder = IRBuilder(ctx);
  builder.SetInsertPoint(&first_block);
  // pointer to a single state
  // GPR array is the first field
  auto *const_zero = ConstantInt::get(ctx, APInt(64, 0));
  auto *array_ptr = builder.CreateGEP(&state, state_arg,
                                      ArrayRef<Value *>{const_zero}, "GPRS");
  auto *reg_info = target_machine.getMCRegisterInfo();
  assert(reg_info);
  // TODO: get GPR reg class index propperly
  constexpr auto gpr_class_idx = 3u;
  auto &gpr_class = reg_info->getRegClass(gpr_class_idx);
  auto sorted_regs = std::set<unsigned>();
  ranges::copy(gpr_class, std::inserter(sorted_regs, sorted_regs.end()));
  for (auto [idx, reg] : ranges::views::enumerate(sorted_regs)) {
    auto *array_idx = ConstantInt::get(ctx, APInt(64, idx));
    auto *reg_dst_addr =
        builder.CreateAlloca(Type::getIntNTy(ctx, 64), /*array size*/ nullptr,
                             reg_info->getName(reg));
    auto *reg_addr = builder.CreateInBoundsGEP(
        array_type, array_ptr, ArrayRef<Value *>{const_zero, array_idx});
    auto *reg_val = builder.CreateLoad(Type::getIntNTy(ctx, 64), reg_addr);
    builder.CreateStore(reg_val, reg_dst_addr);
    rmap.try_emplace(reg, reg_dst_addr);
  }
}

auto *generate_function_object(Module &m, MachineFunction &mf, reg2vals &rmap,
                               MachineModuleInfo &mmi, StructType &state) {
  auto *ret_type = Type::getVoidTy(m.getContext());
  auto *func_type = FunctionType::get(
      ret_type, ArrayRef<Type *>{PointerType::getUnqual(m.getContext())},
      /* is var arg */ false);
  auto name_str = mf.getName().str();
  mf.getFunction().setName(mf.getName().str() + ".old");
  auto *func =
      Function::Create(func_type, Function::ExternalLinkage, name_str, m);
  return func;
}

void save_registers(BasicBlock &block, BasicBlock::iterator pos, reg2vals &rmap,
                    StructType &state) {
  auto &ctx = block.getContext();
  IRBuilder builder(block.getContext());
  builder.SetInsertPoint(pos);
  constexpr auto gpr_array_idx = 0u;
  Value *state_arg = block.getParent()->getArg(gpr_array_idx);
  auto *array_type = *state.element_begin();
  // GPR array is the first field
  auto *const_zero = ConstantInt::get(ctx, APInt(64, 0));
  auto *array_ptr = builder.CreateGEP(&state, state_arg,
                                      ArrayRef<Value *>{const_zero}, "GPRS");
  for (auto &&[idx, val] : rmap | ranges::views::enumerate) {
    auto *array_idx = ConstantInt::get(ctx, APInt(64, idx));
    auto &&[reg, addr] = val;
    auto *reg_val = builder.CreateLoad(Type::getIntNTy(ctx, 64), addr);
    auto *reg_dst_addr = builder.CreateInBoundsGEP(
        array_type, array_ptr, ArrayRef<Value *>{const_zero, array_idx});
    builder.CreateStore(reg_val, reg_dst_addr);
  }
}

void save_registers_before_return(Function &func, reg2vals &rmap, mbb2bb &m2b,
                                  StructType &state) {
  for (auto &block : func) {
    auto ret = ranges::find_if(
        block, [](auto &inst) { return isa<ReturnInst>(inst); });
    if (ret == block.end())
      continue;
    save_registers(block, ret, rmap, state);
  }
}

auto generate_function(Module &m, MachineFunction &mf, const instr_impl &instrs,
                       MachineModuleInfo &mmi, const target &tgt,
                       StructType &state) {
  reg2vals rmap;
  mbb2bb m2b;
  auto *func = generate_function_object(m, mf, rmap, mmi, state);
  assert(func);
  auto &dst = mmi.getOrCreateMachineFunction(*func);
  create_basic_blocks_for_mfunc(mf, dst, m2b);
  materialize_registers(mf, *func, rmap, mmi.getTarget(), state);
  for (auto &mbb : dst)
    fill_ir_for_bb(mbb, rmap, instrs, mmi.getTarget(), tgt, m2b);
  save_registers_before_return(*func, rmap, m2b, state);
}

std::string get_instruction_name(const MachineInstr &minst,
                                 const MCInstrInfo &instr_info) {
  return instr_info.getName(minst.getOpcode()).str();
}

StructType &create_state_type(LLVMContext &ctx) {
  // FIXME: register number and width should not be hardcoded. They can be
  // determined from input MIR.
  // GPR registers
  auto *array_type =
      ArrayType::get(Type::getIntNTy(ctx, 64), /*register number*/ 32);
  auto *struct_type =
      StructType::create(ctx, ArrayRef<Type *>{array_type}, "register_state");
  assert(struct_type);
  return *struct_type;
}

PreservedAnalyses block_ir_builder_pass::run(Module &m,
                                             ModuleAnalysisManager &mam) {
  std::set<Function *> target_functions;
  transform(m, std::inserter(target_functions, target_functions.end()),
            [](auto &f) { return &f; });
  fill_module_with_instrs(m, instrs);
  auto m2b = mbb2bb{};
  auto &mmi = mam.getResult<MachineModuleAnalysis>(m).getMMI();
  auto &state = create_state_type(m.getContext());
  for (auto *f : target_functions) {
    auto &mf = mmi.getOrCreateMachineFunction(*f);
    generate_function(m, mf, instrs, mmi, tgt, state);
  }
  for (auto *f : target_functions)
    f->eraseFromParent();
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
  if (minst.isReturn()) {
    assert(minst.getNumOperands() <= 1);
    // non-void case
    if (minst.getNumOperands() == 1) {
      auto ret = minst.getOperand(0);
      assert(ret.isReg());
      auto *reg_addr = rmap[ret.getReg()];
      auto *reg_val = builder.CreateLoad(Type::getIntNTy(ctx, 64), reg_addr);
      return builder.CreateRet(reg_val);
    }
    return builder.CreateRetVoid();
  }
  auto &instr_module = instrs.get(name);
  auto *func = instr_module.getFunction(name);
  if (!func)
    throw std::runtime_error("Could not find \"" + name + "\" in module");
  std::vector<Value *> args;
  for (auto &&use : minst.uses() | ranges::views::reverse) {
    assert(use.isUse());
    if (!use.isReg())
      throw std::runtime_error("Only register operands are supported");
    auto *reg_addr = rmap[use.getReg()];
    auto *reg_val = builder.CreateLoad(Type::getIntNTy(ctx, 64), reg_addr);
    args.push_back(reg_val);
  }
  auto *call = builder.CreateCall(func->getFunctionType(), func, args);
  for (auto &def : minst.defs()) {
    assert(def.isDef());
    if (!def.isReg())
      throw std::runtime_error("Only register operands are supported");
    auto *reg_addr = rmap[def.getReg()];
    builder.CreateStore(call, reg_addr);
  }
  return call;
}

void fill_ir_for_bb(MachineBasicBlock &mbb, reg2vals &rmap,
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
