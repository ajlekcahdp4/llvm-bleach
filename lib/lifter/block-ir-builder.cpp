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
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/Utils/Cloning.h>

#include <expected>
#include <fstream>
#include <iostream>
#include <ranges>
#include <set>

namespace bleach {
llvm::cl::OptionCategory options("llvm-bleach lifter options");
} // namespace bleach
namespace bleach::lifter {
namespace ranges = std::ranges;
using namespace llvm;
static cl::opt<bool>
    assume_function_nop("assume-function-nop",
                        cl::desc("Assume called functions don't change state"),
                        cl::cat(options));
static cl::opt<std::string>
    dump_struct_def_option("state-struct-file",
                           cl::desc("File to dump state struct definition to"));

static cl::opt<unsigned>
    stack_size_option("stack-size", cl::desc("Stack size for array (bytes)"),
                      cl::init(8000));

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
  for (auto &mbb : dst) {
    for (auto *succ : mbb.successors())
      mbb.ReplaceUsesOfBlockWith(succ, block_map.at(succ));
  }
  for (auto &block : dst.getFunction()) {
    for (auto &inst : block)
      if (auto *call = dyn_cast<CallInst>(&inst)) {
        if (auto *callee = call->getCalledFunction();
            callee && callee->getName() == dst.getName()) {
          call->setCalledFunction(&dst.getFunction());
        }
      }
  }
  assert(ranges::all_of(dst, [](auto &mbb) {
    return ranges::all_of(mbb.successors(), [&mbb](auto *succ) {
      return succ->getParent() == mbb.getParent();
    });
  }));

  for (auto &mbb : dst) {
    for (auto &minst : mbb) {
      if (minst.isBranch()) {
        auto dst_mbb = llvm::find_if(minst.operands(),
                                     [](auto &op) { return op.isMBB(); });
        assert(dst_mbb != minst.operands_end());
        assert(!block_map.count(dst_mbb->getMBB()));
        assert(dst_mbb->getMBB()->getParent() == &dst);
      }
    }
  }
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

auto get_current_state(Function &func) -> Value * {
  constexpr auto gpr_array_idx = 0u;
  return func.getArg(gpr_array_idx);
}

void materialize_registers(MachineFunction &mf, Function &func, reg2vals &rmap,
                           const LLVMTargetMachine &target_machine,
                           StructType &state, const register_stats &reg_stats) {
  if (mf.empty())
    return;
  auto &ctx = func.getContext();
  auto *state_arg = get_current_state(func);
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
  auto *stinfo = target_machine.getSubtargetImpl(func);
  assert(stinfo);
  auto *rinfo = stinfo->getRegisterInfo();
  assert(rinfo);
  for (auto &reg_class : reg_stats) {
    auto sorted_regs = std::set<unsigned>();
    ranges::copy(reg_class, std::inserter(sorted_regs, sorted_regs.end()));
    for (auto [idx, reg] : ranges::views::enumerate(sorted_regs)) {
      auto *array_idx = ConstantInt::get(ctx, APInt(64, idx));
      auto *reg_dst_addr = builder.CreateAlloca(
          Type::getIntNTy(ctx, reg_class.get_register_size()),
          /*array size*/ nullptr);
      auto *reg_addr = builder.CreateInBoundsGEP(
          array_type, array_ptr, ArrayRef<Value *>{const_zero, array_idx});
      auto *reg_val = builder.CreateLoad(
          Type::getIntNTy(ctx, reg_class.get_register_size()), reg_addr);
      builder.CreateStore(reg_val, reg_dst_addr);
      rmap.try_emplace(reg, reg_dst_addr);
    }
  }
}

auto *generate_function_object(Module &m, MachineFunction &mf) {
  auto *ret_type = mf.getFunction().getFunctionType()->getReturnType();
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
                    const LLVMTargetMachine &tmachine, StructType &state,
                    const register_stats &reg_stats) {
  auto &ctx = block.getContext();
  auto *stinfo = tmachine.getSubtargetImpl(*block.getParent());
  assert(stinfo);
  auto *rinfo = stinfo->getRegisterInfo();
  assert(rinfo);
  IRBuilder builder(block.getContext());
  if (pos == block.end())
    builder.SetInsertPoint(&block);
  else
    builder.SetInsertPoint(pos);
  auto *state_arg = get_current_state(*block.getParent());
  auto *array_type = *state.element_begin();
  // GPR array is the first field
  auto *const_zero = ConstantInt::get(ctx, APInt(64, 0));
  auto *array_ptr = builder.CreateGEP(&state, state_arg,
                                      ArrayRef<Value *>{const_zero}, "GPRS");
  for (auto &&[idx, val] : rmap | ranges::views::enumerate) {
    auto *array_idx = ConstantInt::get(ctx, APInt(64, idx));
    auto &&[reg, addr] = val;
    auto &rclass = reg_stats.get_register_class_for(reg);
    auto *reg_val = builder.CreateLoad(
        Type::getIntNTy(ctx, rclass.get_register_size()), addr);
    auto *reg_dst_addr = builder.CreateInBoundsGEP(
        array_type, array_ptr, ArrayRef<Value *>{const_zero, array_idx});
    builder.CreateStore(reg_val, reg_dst_addr);
  }
}

void load_registers(BasicBlock &block, BasicBlock::iterator pos, reg2vals &rmap,
                    const LLVMTargetMachine &tmachine, StructType &state,
                    const register_stats &reg_stats) {
  auto &ctx = block.getContext();
  auto *stinfo = tmachine.getSubtargetImpl(*block.getParent());
  assert(stinfo);
  auto *rinfo = stinfo->getRegisterInfo();
  assert(rinfo);
  IRBuilder builder(block.getContext());
  if (pos == block.end())
    builder.SetInsertPoint(&block);
  else
    builder.SetInsertPoint(pos);
  auto *state_arg = get_current_state(*block.getParent());
  auto *array_type = *state.element_begin();
  // GPR array is the first field
  auto *const_zero = ConstantInt::get(ctx, APInt(64, 0));
  auto *array_ptr = builder.CreateGEP(&state, state_arg,
                                      ArrayRef<Value *>{const_zero}, "GPRS");
  for (auto &&[idx, val] : rmap | ranges::views::enumerate) {
    auto *array_idx = ConstantInt::get(ctx, APInt(64, idx));
    auto &&[reg, addr] = val;
    auto &rclass = reg_stats.get_register_class_for(reg);
    auto *reg_src_addr = builder.CreateInBoundsGEP(
        array_type, array_ptr, ArrayRef<Value *>{const_zero, array_idx});
    auto *reg_val = builder.CreateLoad(
        Type::getIntNTy(ctx, rclass.get_register_size()), reg_src_addr);
    builder.CreateStore(reg_val, addr);
  }
}

void save_registers_before_return(Function &func, reg2vals &rmap,
                                  const LLVMTargetMachine &tmachine,
                                  StructType &state,
                                  const register_stats &reg_stats) {
  for (auto &block : func) {
    auto ret = ranges::find_if(
        block, [](auto &inst) { return isa<ReturnInst>(inst); });
    if (ret == block.end())
      continue;
    save_registers(block, ret, rmap, tmachine, state, reg_stats);
  }
}

struct clone_function_result final {
  mbb2bb blocks;
  MachineFunction *mfunc = nullptr;
};

auto generate_function(MachineFunction &mf, clone_function_result &dst_info,
                       const instr_impl &instrs, MachineModuleInfo &mmi,
                       StructType &state, const register_stats &reg_stats) {
  reg2vals rmap;
  auto &func = dst_info.mfunc->getFunction();
  materialize_registers(mf, func, rmap, mmi.getTarget(), state, reg_stats);
  for (auto &mbb : *dst_info.mfunc)
    fill_ir_for_bb(mbb, rmap, instrs, mmi.getTarget(), dst_info.blocks, state,
                   reg_stats);
  save_registers_before_return(func, rmap, mmi.getTarget(), state, reg_stats);
}

std::string get_instruction_name(const MachineInstr &minst,
                                 const MCInstrInfo &instr_info) {
  return instr_info.getName(minst.getOpcode()).str();
}

StructType &create_state_type(LLVMContext &ctx,
                              const LLVMTargetMachine &tmachine,
                              size_t stack_size, unsigned regs_num,
                              unsigned reg_bitsize) {
  auto *gprs_array_type =
      ArrayType::get(Type::getIntNTy(ctx, reg_bitsize), regs_num);
  auto *stack_type =
      ArrayType::get(Type::getIntNTy(ctx, reg_bitsize), stack_size);
  auto *struct_type = StructType::create(
      ctx, ArrayRef<Type *>{gprs_array_type, stack_type}, "register_state");
  assert(struct_type);
  return *struct_type;
}

auto clone_machine_function(Module &m, MachineModuleInfo &mmi,
                            MachineFunction &mfunc) {
  auto *new_func = generate_function_object(m, mfunc);
  assert(new_func);
  auto &new_mfunc = mmi.getOrCreateMachineFunction(*new_func);
  clone_function_result res = {{}, &new_mfunc};
  create_basic_blocks_for_mfunc(mfunc, new_mfunc, res.blocks);
  // mfunc.getFunction().eraseFromParent();
  return res;
}

void collect_register_stats_for(const MachineFunction &mf,
                                const LLVMTargetMachine &tmachine,
                                register_stats &stats) {
  auto *stinfo = tmachine.getSubtargetImpl(mf.getFunction());
  assert(stinfo);
  auto *rinfo = stinfo->getRegisterInfo();
  assert(rinfo);
  for (auto &mblock : mf) {
    for (auto &minst : mblock) {
      for (auto &mop : minst.operands()) {
        if (mop.isReg()) {
          StringRef rname = rinfo->getRegAsmName(mop.getReg());
          auto matching = ranges::find_if(stats, [rname](auto &rclass) -> bool {
            return rclass.get_regex().match(rname);
          });
          if (matching == stats.end())
            throw std::runtime_error("No register class found to match: \"" +
                                     rname.str() + "\"");
          matching->add_reg(mop.getReg());
        }
      }
    }
  }
}

void assign_register_classes(const TargetSubtargetInfo &stinfo,
                             register_stats &stats) {
  auto *rinfo = stinfo.getRegisterInfo();
  assert(rinfo);
  for (auto &&[idx, reg_class] : enumerate(stats)) {
    auto found = find_if(rinfo->regclasses(), [&reg_class](auto &rclass) {
      return all_of(reg_class,
                    [&rclass](auto reg) { return is_contained(*rclass, reg); });
    });
    if (found == rinfo->regclass_end()) {
      throw std::runtime_error(
          "Could not find llvm register class for specified register class: " +
          std::to_string(idx));
    }
    reg_class.set_rclass(*found, rinfo->getRegSizeInBits(**found));
    for (auto reg : **found) {
      if (reg_class.get_regex().sub("", rinfo->getRegAsmName(reg)).empty())
        reg_class.add_reg(reg);
    }
  }
}

register_stats collect_register_stats(const instr_impl &instr, Module &m,
                                      MachineModuleInfo &mmi) {
  auto &rclasses = instr.get_regclasses();
  if (rclasses.empty())
    throw std::runtime_error(
        "register-classes were not specified in input YAML");
  register_stats stats(rclasses.begin(), rclasses.end());
  for (auto &f : m) {
    auto &mf = mmi.getOrCreateMachineFunction(f);
    collect_register_stats_for(mf, mmi.getTarget(), stats);
  }
  assert(!m.empty());
  auto *stinfo = mmi.getTarget().getSubtargetImpl(*m.begin());
  assert(stinfo);
  assign_register_classes(*stinfo, stats);
  return stats;
}

std::string get_state_struct_definition(const register_stats &regs,
                                        size_t stack_size) {
  auto num_regs = std::accumulate(
      regs.begin(), regs.end(), 0u,
      [](auto acc, auto &val) -> size_t { return acc + val.size(); });
  assert(num_regs);
  // FIXME: consider all regs are of the same size
  auto &rclass = *regs.begin();
  auto reg_size = rclass.get_register_size();
  std::string strct;
  raw_string_ostream ss(strct);
  ss << "struct register_state {\n";
  ss << formatv("  int{0}_t regs[{1}];\n", reg_size, num_regs);
  ss << formatv("  int{0}_t stack[{1}];\n", reg_size, stack_size);
  ss << "};";
  return strct;
}

PreservedAnalyses block_ir_builder_pass::run(Module &m,
                                             ModuleAnalysisManager &mam) {
  auto &mmi = mam.getResult<MachineModuleAnalysis>(m).getMMI();
  auto reg_stats = collect_register_stats(instrs, m, mmi);
  std::set<Function *> target_functions;
  ranges::transform(m, std::inserter(target_functions, target_functions.end()),
                    [](auto &f) { return &f; });
  fill_module_with_instrs(m, instrs);
  std::unordered_map<MachineFunction *, clone_function_result> funcs;
  for (auto *f : target_functions) {
    auto &mf = mmi.getOrCreateMachineFunction(*f);
    funcs.try_emplace(&mf, clone_machine_function(m, mmi, mf));
  }
  for (auto &&[oldf, func_info] : funcs) {
    for (auto &block : *func_info.mfunc) {
      for (auto &inst : block) {
        if (!inst.isCall())
          continue;
        auto &&callee_op = inst.getOperand(0);
        assert(callee_op.isGlobal());
        auto *callee_global = callee_op.getGlobal();
        auto *callee = dyn_cast<Function>(callee_global);
        assert(callee);
        auto *new_callee = funcs.at(mmi.getMachineFunction(*callee)).mfunc;
        callee_op.ChangeToGA(&new_callee->getFunction(), /*Offset=*/0);
      }
    }
  }

  auto stack_size_bytes = stack_size_option.getValue();
  auto num_regs = std::accumulate(
      reg_stats.begin(), reg_stats.end(), 0u,
      [](auto acc, auto &val) -> size_t { return acc + val.size(); });
  // FIXME: consider all regs are of the same size
  auto &rclass = *reg_stats.begin();
  auto reg_size = rclass.get_register_size();
  auto reg_size_bytes = reg_size / CHAR_BIT;
  auto stack_size_elems = stack_size_bytes / reg_size_bytes;
  auto &state = create_state_type(m.getContext(), mmi.getTarget(),
                                  stack_size_elems, num_regs, reg_size);
  if (!dump_struct_def_option.empty()) {
    auto struct_def_str =
        get_state_struct_definition(reg_stats, stack_size_elems);
    if (dump_struct_def_option == "-") {
      std::cout << struct_def_str << std::endl;
    } else {
      std::fstream fs(dump_struct_def_option.getValue(), std::fstream::out);
      if (!fs.is_open())
        throw std::runtime_error("Could not open file \"" +
                                 dump_struct_def_option.getValue() + "\"");
      fs << struct_def_str << std::endl;
    }
  }

  for (auto &&[oldf, func_info] : funcs)
    generate_function(*oldf, func_info, instrs, mmi, state, reg_stats);

  for (auto *f : target_functions)
    f->eraseFromParent();
  return PreservedAnalyses::none();
}

auto generate_jump(const MachineInstr &minst, IRBuilder<> &builder,
                   const mbb2bb &m2b) -> Instruction * {
  assert(minst.isBranch());
  auto mbb_op =
      llvm::find_if(minst.operands(), [](auto &op) { return op.isMBB(); });
  if (minst.getIterator() != minst.getParent()->begin()) {
    if (std::prev(minst.getIterator())->isConditionalBranch())
      return nullptr;
  }
  assert(mbb_op != minst.operands_end());
  auto *mbb = mbb_op->getMBB();
  assert(mbb);
  auto *target_bb = m2b[mbb];
  assert(target_bb);
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
  if (next != minst.getParent()->end()) {
    if (next->isUnconditionalBranch()) {
      auto &op = next->getOperand(0);
      assert(op.isMBB());
      return m2b[op.getMBB()];
    }
    auto *func = m2b[minst.getParent()]->getParent();
    return BasicBlock::Create(func->getContext(), "", func);
  }
  return m2b[std::addressof(*std::next(minst.getParent()->getIterator()))];
}

auto operand_to_value(const MachineOperand &mop, BasicBlock &block,
                      const LLVMTargetMachine &tmachine, reg2vals &rmap,
                      const register_stats &reg_stats) -> Value * {
  IRBuilder builder(block.getContext());
  builder.SetInsertPoint(&block);
  if (mop.isReg()) {
    auto *stinfo = tmachine.getSubtargetImpl(*block.getParent());
    assert(stinfo);
    auto *rinfo = stinfo->getRegisterInfo();
    assert(rinfo);
    assert(mop.isUse());
    auto &rclass = reg_stats.get_register_class_for(mop.getReg());
    auto *reg_addr = rmap[mop.getReg()];
    auto *reg_val = builder.CreateLoad(
        Type::getIntNTy(block.getContext(), rclass.get_register_size()),
        reg_addr);
    return reg_val;
  }
  if (mop.isImm()) {
    auto val = mop.getImm();
    auto *imm = ConstantInt::get(block.getContext(), APInt(64, val));
    return imm;
  }
  throw std::runtime_error("Unsupported operand type");
}

auto generate_branch(const MachineInstr &minst, BasicBlock &bb,
                     IRBuilder<> &builder, reg2vals &rmap,
                     const LLVMTargetMachine &target_machine, const mbb2bb &m2b,
                     const register_stats &reg_stats) -> Instruction * {
  auto *iinfo = target_machine.getMCInstrInfo();
  auto name = get_instruction_name(minst, *iinfo);
  auto *m = bb.getParent()->getParent();
  auto *func = m->getFunction(name);
  if (!func)
    throw std::runtime_error("Could not find \"" + name + "\" in module");
  auto op_to_val = [&](auto &mop) {
    return operand_to_value(mop, bb, target_machine, rmap, reg_stats);
  };
  auto values = minst.uses() |
                ranges::views::filter([](auto &&mop) { return !mop.isMBB(); }) |
                ranges::views::transform(op_to_val);
  std::vector<Value *> args(values.begin(), values.end());
  auto *cond = builder.CreateCall(func->getFunctionType(), func, args);
  auto *if_true = get_if_true_block(minst, m2b);
  auto *if_false = get_if_false_block(minst, m2b);
  auto *br = builder.CreateCondBr(cond, if_true, if_false);
  builder.SetInsertPoint(if_false);
  return br;
}

std::optional<unsigned> find_register_by_name(const MCRegisterInfo *reg_info,
                                              StringRef name) {
  for (auto &&rclass : reg_info->regclasses()) {
    auto reg_it = ranges::find_if(
        rclass, [&](auto &reg) { return name == reg_info->getName(reg); });
    if (reg_it != rclass.end())
      return *reg_it;
  }
  return std::nullopt;
}

static auto
write_value_to_register(Value *val, MCRegister reg, IRBuilder<> &builder,
                        reg2vals &rmap, const instr_impl &instrs,
                        const LLVMTargetMachine &tmachine,
                        const register_stats &reg_stats) -> Value * {
  auto *reg_addr = rmap.at(reg);
  auto *reg_info = tmachine.getMCRegisterInfo();
  auto &const_regs = instrs.get_const_regs();
  auto const_regs_vals = std::unordered_map<unsigned, uint64_t>();
  ranges::transform(
      const_regs, std::inserter(const_regs_vals, const_regs_vals.end()),
      [reg_info](auto &e) -> std::pair<unsigned, uint64_t> {
        auto special_reg = find_register_by_name(reg_info, e.name);
        if (!special_reg)
          throw std::runtime_error(
              "Could not find constant register under the name \"" + e.name +
              "\"");
        return {*special_reg, e.value};
      });
  auto *stinfo =
      tmachine.getSubtargetImpl(*builder.saveIP().getBlock()->getParent());
  assert(stinfo);
  auto *rinfo = stinfo->getRegisterInfo();
  assert(rinfo);
  auto &rclass = reg_stats.get_register_class_for(reg);
  auto found = ranges::find_if(
      const_regs, [&](auto &r) { return r.name == reg_info->getName(reg); });
  if (found != const_regs.end()) {
    return builder.CreateStore(
        ConstantInt::get(val->getContext(), APInt(rclass.get_register_size(),
                                                  const_regs_vals.at(reg))),
        reg_addr);
  }
  builder.CreateStore(val, reg_addr);
  return val;
}

static auto read_register_value(MCRegister reg,
                                const LLVMTargetMachine &tmachine,
                                IRBuilder<> &builder, reg2vals &rmap,
                                const register_stats &reg_stats) -> Value * {
  auto *reg_addr = rmap.at(reg);
  auto &ctx = builder.getContext();
  auto *stinfo =
      tmachine.getSubtargetImpl(*builder.saveIP().getBlock()->getParent());
  assert(stinfo);
  auto *rinfo = stinfo->getRegisterInfo();
  assert(rinfo);
  auto &rclass = reg_stats.get_register_class_for(reg);
  return builder.CreateLoad(Type::getIntNTy(ctx, rclass.get_register_size()),
                            reg_addr);
}
static auto generate_return(const MachineInstr &minst,
                            const LLVMTargetMachine &tmachine,
                            IRBuilder<> &builder, reg2vals &rmap,
                            const register_stats &reg_stats) -> Value * {
  assert(minst.getNumOperands() <= 1);
  // non-void case
  if (minst.getNumOperands() == 1) {
    auto ret = minst.getOperand(0);
    assert(ret.isReg());
    auto *reg_val =
        read_register_value(ret.getReg(), tmachine, builder, rmap, reg_stats);
    return builder.CreateRet(reg_val);
  }
  return builder.CreateRetVoid();
}

static auto generate_call(const MachineInstr &minst, IRBuilder<> &builder,
                          BasicBlock &bb, reg2vals &rmap,
                          const LLVMTargetMachine &tmachine, StructType &state,
                          const register_stats &reg_stats) -> Value * {
  assert(minst.getNumOperands() >= 1);
  auto op = minst.getOperand(0);
  assert(op.isGlobal());
  auto *callee = const_cast<Function *>(dyn_cast<Function>(op.getGlobal()));
  assert(callee);
  auto *call =
      builder.CreateCall(callee->getFunctionType(), callee,
                         ArrayRef<Value *>{get_current_state(*bb.getParent())});
  save_registers(bb, call->getIterator(), rmap, tmachine, state, reg_stats);
  if (!assume_function_nop)
    load_registers(bb, std::next(call->getIterator()), rmap, tmachine, state,
                   reg_stats);
  return call;
}

std::optional<unsigned> get_stack_pointer(const instr_impl &instrs,
                                          const LLVMTargetMachine &tmachine) {
  auto *reg_info = tmachine.getMCRegisterInfo();
  auto sp_reg = find_register_by_name(reg_info, instrs.get_stack_pointer());
  return sp_reg;
}

auto get_stack_pointer_value(const instr_impl &instrs, reg2vals &rmap,
                             IRBuilder<> &builder,
                             const LLVMTargetMachine &tmachine,
                             const register_stats &reg_stats) -> Value * {
  auto sp_reg = get_stack_pointer(instrs, tmachine);
  if (!sp_reg)
    throw std::runtime_error("Could not find stack pointer under the name \"" +
                             instrs.get_stack_pointer().str() + "\"");
  return read_register_value(*sp_reg, tmachine, builder, rmap, reg_stats);
}

static auto generate_load_store_from_stack(
    const MachineInstr &minst, IRBuilder<> &builder, BasicBlock &bb,
    reg2vals &rmap, const instr_impl &instrs,
    const LLVMTargetMachine &target_machine, StructType &state,
    const register_stats &reg_stats) -> Value * {
  auto &ctx = bb.getContext();
  auto uses = minst.uses();
  auto sp_reg = get_stack_pointer(instrs, target_machine);
  if (!sp_reg)
    throw std::runtime_error("Could not find stack pointer under the name \"" +
                             instrs.get_stack_pointer().str() + "\"");
  auto &rclass = reg_stats.get_register_class_for(*sp_reg);
  auto reg_bitsize = rclass.get_register_size();
  auto reg_bytesize = reg_bitsize / CHAR_BIT;
  auto offset_it =
      ranges::find_if(uses, [](auto &&mop) { return mop.isImm(); });
  auto *offset = [&] -> Value * {
    if (offset_it == uses.end())
      return ConstantInt::get(ctx, APInt(64, 0));
    return ConstantInt::get(ctx,
                            APInt(64, -offset_it->getImm() / reg_bytesize));
  }();
  auto *sp =
      get_stack_pointer_value(instrs, rmap, builder, target_machine, reg_stats);
  auto *idx = builder.CreateAdd(sp, offset);
  auto *state_arg = get_current_state(*bb.getParent());
  auto *stack_type = *std::next(state.element_begin());
  auto *stack_addr =
      builder.CreateGEP(&state, state_arg,
                        ArrayRef<Value *>{ConstantInt::get(ctx, APInt(64, 1))});
  auto *addr = builder.CreateInBoundsGEP(
      stack_type, stack_addr,
      ArrayRef<Value *>{ConstantInt::get(ctx, APInt(64, 0)), idx});
  auto *iinfo = target_machine.getMCInstrInfo();
  auto name = get_instruction_name(minst, *iinfo);
  auto *m = bb.getParent()->getParent();
  auto *func = m->getFunction(name);
  if (!func)
    throw std::runtime_error("Could not find \"" + name + "\" in module");
  auto *inserted = [&] -> Value * {
    // Loading value from stack
    if (minst.mayLoad()) {
      if (minst.mayStore())
        throw std::runtime_error("Unsupported stack manipulation. Instruction "
                                 "can both load and store");
      auto *ret_type = func->getFunctionType()->getReturnType();
      return builder.CreateLoad(ret_type, addr);
    }
    assert(minst.mayStore());
    auto op_to_val = [&](auto &mop) {
      return operand_to_value(mop, bb, target_machine, rmap, reg_stats);
    };
    // Value, addr reg and offset expected
    assert(ranges::size(minst.uses()) == 3);
    // Skip source register and offset.
    auto &&value = op_to_val(*minst.uses().begin());
    return builder.CreateStore(value, addr);
  }();
  // save result to register
  if (!minst.defs().empty())
    builder.CreateStore(inserted, rmap.at(minst.defs().begin()->getReg()));
  return inserted;
}

auto generate_stack_pointer_modification(
    const MachineInstr &minst, IRBuilder<> &builder, BasicBlock &bb,
    const LLVMTargetMachine &tmachine, reg2vals &rmap, const instr_impl &instrs,
    const register_stats &reg_stats) -> Value * {
  auto &ctx = bb.getContext();
  auto *reg_info = tmachine.getMCRegisterInfo();
  auto sp_reg = find_register_by_name(reg_info, instrs.get_stack_pointer());
  if (!sp_reg)
    throw std::runtime_error("Could not find stack pointer under the name \"" +
                             instrs.get_stack_pointer().str() + "\"");
  auto uses = minst.uses();
  auto imm = ranges::find_if(uses, [](auto &mop) { return mop.isImm(); });
  if (imm == uses.end()) {
    std::string err;
    raw_string_ostream os(err);
    os << "SP is expected to be used only as a stack pointer: \"";
    minst.print(os);
    os << "\"";
    throw std::runtime_error(err);
  }
  std::vector<Value *> args;
  ranges::transform(
      minst.uses(), std::back_inserter(args), [&](auto &mop) -> Value * {
        if (!mop.isImm() || mop.getImm() != imm->getImm())
          return operand_to_value(mop, bb, tmachine, rmap, reg_stats);
        auto val = mop.getImm();
        return ConstantInt::get(ctx, APInt(64, -val / 8));
      });
  auto *iinfo = tmachine.getMCInstrInfo();
  auto name = get_instruction_name(minst, *iinfo);
  auto *m = bb.getParent()->getParent();
  auto *func = m->getFunction(name);
  auto *call = builder.CreateCall(func->getFunctionType(), func, args);
  for (auto &def : minst.defs()) {
    assert(def.isDef());
    if (!def.isReg())
      throw std::runtime_error("Only register operands are supported");
    write_value_to_register(call, def.getReg(), builder, rmap, instrs, tmachine,
                            reg_stats);
  }
  return call;
}

// Operation is considered a stack manipulation if it is a load or store and
// its source register is stack pointer
bool is_stack_manipulation(const MachineInstr &minst, const instr_impl &instrs,
                           const LLVMTargetMachine &tmachine) {
  if (!minst.mayLoadOrStore())
    return false;
  auto sp_reg_opt = get_stack_pointer(instrs, tmachine);
  if (!sp_reg_opt)
    throw std::runtime_error("Could not find stack pointer under the name \"" +
                             instrs.get_stack_pointer().str() + "\"");
  auto sp_reg = *sp_reg_opt;
  auto operands = minst.uses();
  auto regops = operands |
                ranges::views::filter([](auto &mop) { return mop.isReg(); }) |
                ranges::views::transform(
                    [](auto &mop) -> unsigned { return mop.getReg(); });
  auto used_reg = ranges::find(regops, sp_reg);
  return used_reg != regops.end();
}

auto generate_instruction(const MachineInstr &minst, BasicBlock &bb,
                          IRBuilder<> &builder, reg2vals &rmap,
                          const instr_impl &instrs,
                          const LLVMTargetMachine &target_machine,
                          const mbb2bb &m2b, StructType &state,
                          const register_stats &reg_stats) -> Value * {
  auto *iinfo = target_machine.getMCInstrInfo();
  auto name = get_instruction_name(minst, *iinfo);
  if (minst.isBranch()) {
    if (minst.isUnconditionalBranch())
      return generate_jump(minst, builder, m2b);
    assert(minst.isConditionalBranch());
    return generate_branch(minst, bb, builder, rmap, target_machine, m2b,
                           reg_stats);
  }
  if (minst.isReturn())
    return generate_return(minst, target_machine, builder, rmap, reg_stats);
  if (minst.isCall())
    return generate_call(minst, builder, bb, rmap, target_machine, state,
                         reg_stats);
  if (minst.mayLoadOrStore()) {
    // Instructions that are not considered stack manipulation are generated
    // just like any other instruction
    if (is_stack_manipulation(minst, instrs, target_machine))
      return generate_load_store_from_stack(minst, builder, bb, rmap, instrs,
                                            target_machine, state, reg_stats);
  }
  auto *reg_info = target_machine.getMCRegisterInfo();
  auto sp_reg = find_register_by_name(reg_info, instrs.get_stack_pointer());
  if (!sp_reg)
    throw std::runtime_error("Could not find stack pointer under the name \"" +
                             instrs.get_stack_pointer().str() + "\"");
  auto *m = bb.getParent()->getParent();
  auto *func = m->getFunction(name);
  if (!func)
    throw std::runtime_error("Could not find \"" + name + "\" in module");
  if (!minst.defs().empty() && minst.defs().begin()->isReg() &&
      minst.defs().begin()->getReg() == *sp_reg)
    return generate_stack_pointer_modification(
        minst, builder, bb, target_machine, rmap, instrs, reg_stats);
  auto op_to_val = [&](auto &mop) {
    return operand_to_value(mop, bb, target_machine, rmap, reg_stats);
  };
  auto values = minst.uses() | ranges::views::transform(op_to_val);
  std::vector<Value *> args(values.begin(), values.end());
  auto *call = builder.CreateCall(func->getFunctionType(), func, args);
  for (auto &def : minst.defs()) {
    assert(def.isDef());
    if (!def.isReg())
      throw std::runtime_error("Only register operands are supported");
    write_value_to_register(call, def.getReg(), builder, rmap, instrs,
                            target_machine, reg_stats);
  }
  return call;
}

void fill_ir_for_bb(MachineBasicBlock &mbb, reg2vals &rmap,
                    const instr_impl &instrs,
                    const LLVMTargetMachine &target_machine, const mbb2bb &m2b,
                    StructType &state, const register_stats &reg_stats) {
  assert(!mbb.empty());
  auto *bb = m2b[&mbb];
  IRBuilder builder(bb->getContext());
  builder.SetInsertPoint(bb);
  assert(bb);
  for (auto &minst : mbb) {
    generate_instruction(minst, *bb, builder, rmap, instrs, target_machine, m2b,
                         state, reg_stats);
  }
  auto last = std::prev(mbb.end());
  if (!last->isBranch() && !last->isReturn()) {
    auto succ = std::next(mbb.getIterator());
    builder.CreateBr(m2b[std::addressof(*succ)]);
  }
}

} // namespace bleach::lifter
