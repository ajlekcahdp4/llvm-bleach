#include "bleach/target/riscv/riscv-target.hpp"

#include <llvm/CodeGen/MachineInstr.h>
#include <llvm/IR/IRBuilder.h>

#define GET_INSTRINFO_ENUM
#include "RISCVGenInstrInfo.inc"
#include "RISCVGenRegisterInfo.inc"
#undef GET_INSTRINFO_ENUM

namespace bleach {
auto riscv_target::create_branch_condition(
    const MachineInstr &minst, BasicBlock &bb,
    const std::unordered_map<unsigned, Value *> &rmap,
    LLVMContext &ctx) const -> Value * {
  IRBuilder builder(ctx);
  builder.SetInsertPoint(&bb);
  auto &&op_1 = minst.getOperand(0);
  auto &&op_2 = minst.getOperand(1);
  assert(op_1.isReg());
  assert(op_2.isReg());
  assert(rmap.contains(op_1.getReg()));
  assert(rmap.contains(op_2.getReg()));
  auto *rs1 = rmap.at(op_1.getReg());
  auto *rs2 = rmap.at(op_2.getReg());
  switch (minst.getOpcode()) {
  case RISCV::BLT:
    return builder.CreateICmpSLT(rs1, rs2);
  case RISCV::BLTU:
    return builder.CreateICmpULT(rs1, rs2);
  case RISCV::BGE:
    return builder.CreateICmpSGE(rs1, rs2);
  case RISCV::BGEU:
    return builder.CreateICmpUGE(rs1, rs2);
  case RISCV::BEQ:
    return builder.CreateICmpEQ(rs1, rs2);
  case RISCV::BNE:
    return builder.CreateICmpNE(rs1, rs2);
  }
  throw std::runtime_error("Unknown Branch Instruction");
}
} // namespace bleach
