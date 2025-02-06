#include "bleach/target/riscv/riscv-target.hpp"

#include <llvm/CodeGen/MachineInstr.h>
#include <llvm/IR/IRBuilder.h>

#define GET_INSTRINFO_ENUM
#include "RISCVGenInstrInfo.inc"
#include "RISCVGenRegisterInfo.inc"
#undef GET_INSTRINFO_ENUM

namespace bleach {} // namespace bleach
