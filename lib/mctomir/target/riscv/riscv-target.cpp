#include "riscv-target.hpp"

#include <llvm/CodeGen/MachineInstrBuilder.h>
#include <llvm/CodeGen/TargetInstrInfo.h>

#define GET_INSTRINFO_ENUM
#define GET_REGINFO_ENUM
#include "RISCVGenInstrInfo.inc"
#include "RISCVGenRegisterInfo.inc"
#undef GET_INSTRINFO_ENUM
#undef GET_REGINFO_ENUM

namespace mctomir {
auto riscv_target::insert_return(MachineBasicBlock &mblock,
                                 MachineBasicBlock::iterator ins,
                                 LLVMTargetMachine &tmachine) const -> bool {
  auto &iinfo = *tmachine.getMCInstrInfo();
  auto &desc = iinfo.get(RISCV::PseudoRET);
  MachineInstrBuilder mib = BuildMI(mblock, mblock.end(), DebugLoc(), desc)
                                .addReg(RISCV::X10, RegState::Implicit);
  return mib;
}

auto riscv_target::is_return(MachineInstr &minst) const -> bool {
  if (minst.getOpcode() == RISCV::C_JR) {
    auto num_ops = minst.getNumOperands();
    if (num_ops != 1)
      return false;
    auto &op = minst.getOperand(0);
    assert(op.isReg());
    return op.getReg() == RISCV::X1;
  }
  if (minst.getOpcode() == RISCV::JALR) {
    auto num_ops = minst.getNumOperands();
    if (num_ops != 3)
      return false;
    auto &dst = minst.getOperand(0);
    assert(dst.isReg());
    if (dst.getReg() != RISCV::X0)
      return false;
    auto &src = minst.getOperand(1);
    assert(src.isReg());
    if (src.getReg() != RISCV::X1)
      return false;
    auto &off = minst.getOperand(2);
    if (!off.isImm())
      return false;
    return off.getImm() == 0;
  }
  return false;
}

} // namespace mctomir
