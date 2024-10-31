#include "bleach/lifter/block-ir-builder.hpp"

#include <llvm/CodeGen/MachineFunction.h>
#include <llvm/CodeGen/MachineModuleInfo.h>
#include <llvm/CodeGen/MachineRegisterInfo.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/Linker/Linker.h>
#include <llvm/MC/MCInstrInfo.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/Utils/Cloning.h>

// TODO: move to target-specific part
#define GET_INSTRINFO_ENUM
#include <llvm/Target/RISCV/RISCVGenInstrInfo.inc>
#undef GET_INSTRINFO_ENUM
namespace bleach::lifter {
using namespace llvm;
void fill_module_with_instrs(Module &m, const instr_impl &instrs) {
  for (auto &&i : instrs) {
    assert(i.ir_module);
    auto module_clone = CloneModule(*i.ir_module);
    Linker::linkModules(m, std::move(module_clone));
  }
}

auto *generate_function_object(Module &m, MachineFunction &mf, reg2vals &rmap) {
  auto *ret_type = Type::getIntNTy(m.getContext(), 64);
  auto &reg_info = mf.getRegInfo();
  std::vector<Type *> args;
  for ([[maybe_unused]] auto &&_ : reg_info.liveins())
    args.push_back(Type::getIntNTy(m.getContext(), 64));
  auto *func_type = FunctionType::get(ret_type, args, /* is var arg */ false);
  auto *func =
      Function::Create(func_type, Function::ExternalLinkage, mf.getName());
  for (auto &&[livein, arg] : zip(reg_info.liveins(), func->args()))
    rmap.try_emplace(livein.first, &arg);
  return func;
}

auto generate_function(Module &m, MachineFunction &mf, const instr_impl &instrs,
                       const LLVMTargetMachine &target_machine) {
  reg2vals rmap;
  auto *func = generate_function_object(m, mf, rmap);
  for (auto &mbb : mf)
    fill_ir_for_bb(mbb, *func, rmap, instrs, target_machine);
}

std::string get_instruction_name(const MachineInstr &minst,
                                 const MCInstrInfo &instr_info) {
  return instr_info.getName(minst.getOpcode()).str();
}

PreservedAnalyses block_ir_builder_pass::run(Module &m,
                                             ModuleAnalysisManager &mam) {
  fill_module_with_instrs(m, instrs);
  auto &mmi = mam.getResult<MachineModuleAnalysis>(m).getMMI();
  for (auto &f : m) {
    auto &mf = mmi.getOrCreateMachineFunction(f);
    generate_function(m, mf, instrs, mmi.getTarget());
  }
  return PreservedAnalyses::none();
}

auto generate_instruction(const MachineInstr &minst, BasicBlock &bb,
                          reg2vals &rmap, const instr_impl &instrs,
                          LLVMContext &ctx,
                          const LLVMTargetMachine &target_machine) {
  auto *iinfo = target_machine.getMCInstrInfo();
  auto name = get_instruction_name(minst, *iinfo);
  IRBuilder builder(ctx);
  builder.SetInsertPoint(&bb);
  auto &instr_module = instrs.get(name);
  auto *func = instr_module.getFunction(name);
  if (!func)
    throw std::runtime_error("Could not find \"" + name + "\" in module");
  std::vector<Value *> args;
  for (auto &&use : minst.uses()) {
    assert(use.isUse());
    if (!use.isReg())
      throw std::runtime_error("Only register operands are supported");
    args.push_back(rmap.at(use.getReg().id()));
  }
  auto *call = builder.CreateCall(func->getFunctionType(), func, args);
  for (auto &def : minst.defs()) {
    assert(def.isDef());
    if (!def.isReg())
      throw std::runtime_error("Only register operands are supported");
    auto reg = def.getReg();
    rmap.try_emplace(reg.id(), call);
  }
}

void fill_ir_for_bb(MachineBasicBlock &mbb, Function &func, reg2vals &rmap,
                    const instr_impl &instrs,
                    const LLVMTargetMachine &target_machine) {
  auto *bb = const_cast<BasicBlock *>(mbb.getBasicBlock());
  assert(bb);
  bb->erase(bb->begin(), bb->end());
  for (auto &minst : mbb) {
    generate_instruction(minst, *bb, rmap, instrs, bb->getContext(),
                         target_machine);
  }
}

} // namespace bleach::lifter
