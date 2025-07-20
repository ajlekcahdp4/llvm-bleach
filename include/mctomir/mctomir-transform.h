#pragma once

#include "mctomir/elf-loader.h"

#include <llvm/ADT/DenseMap.h>
#include <llvm/CodeGen/MachineBasicBlock.h>
#include <llvm/CodeGen/MachineFunction.h>
#include <llvm/CodeGen/MachineInstr.h>
#include <llvm/CodeGen/MachineInstrBuilder.h>
#include <llvm/CodeGen/MachineModuleInfo.h>
#include <llvm/CodeGen/MachineOperand.h>
#include <llvm/CodeGen/TargetFrameLowering.h>
#include <llvm/CodeGen/TargetInstrInfo.h>
#include <llvm/CodeGen/TargetRegisterInfo.h>
#include <llvm/CodeGen/TargetSubtargetInfo.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/MC/MCAsmInfo.h>
#include <llvm/MC/MCContext.h>
#include <llvm/MC/MCDisassembler/MCDisassembler.h>
#include <llvm/MC/MCInst.h>
#include <llvm/MC/MCInstPrinter.h>
#include <llvm/MC/MCSymbol.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>

#include <memory>
#include <vector>

namespace mctomir {
using namespace llvm;
using namespace llvm::object;

struct translated_inst {
  uint64_t address;    // Original address
  MCInst inst;         // MC instruction
  MachineInstr *minst; // Machine instruction (owned by MachineFunction)

  translated_inst(uint64_t addr, const MCInst &i)
      : address(addr), inst(i), minst(nullptr) {}
};

struct block_info {
  uint64_t start_address;
  uint64_t end_address;
  MachineBasicBlock *mbb;
  std::vector<translated_inst *> instrs;

  block_info(uint64_t start)
      : start_address(start), end_address(0), mbb(nullptr) {}
};

struct function_info {
  std::vector<translated_inst> insts;
  std::vector<block_info> blocks;
  uint64_t start = 0;
  uint64_t end = 0;
  std::string name;
  Function *func = nullptr;
  MachineFunction *mfunc = nullptr;
};

class translator_t {
public:
  translator_t();
  ~translator_t();

  Error initialize(StringRef triple_name, SubtargetFeatures features);

  Error process_disassembly(SectionRef section, StringRef contents);

  Error add_instruction(uint64_t address, const MCInst &inst);

  Error finalize();

  Error write_mir(StringRef output_filename) const;

  void print_mir(raw_ostream &os) const;

private:
  std::unique_ptr<LLVMTargetMachine> tmachine;
  std::unique_ptr<MachineModuleInfo> mmi;
  std::unique_ptr<MCContext> mcctx;
  std::unique_ptr<MCInstrAnalysis> mi_analysis;
  const TargetInstrInfo *tii;
  const TargetRegisterInfo *tri;
  const TargetSubtargetInfo *tsi;

  std::unique_ptr<LLVMContext> ctx;
  std::unique_ptr<Module> mod;

  std::vector<function_info> funcs;
  DenseMap<uint64_t, MachineBasicBlock *> address_to_mbb;

  Error create_module_and_function(StringRef triple_name,
                                   SubtargetFeatures features);
  Error identify_basic_blocks();
  Error create_machine_basic_blocks();
  Error translate_instructions();
  Error link_machine_basic_blocks();

  MachineInstr *convert_to_machine_instr(const MCInst &inst, uint64_t address);
  MachineOperand convert_operand(const MCOperand &op, const MCInst &inst);
  bool is_terminator(const MCInst &inst);
  bool is_branch(const MCInst &inst);
  bool is_call(const MCInst &inst);
  bool is_return(const MCInst &inst);
  uint64_t get_branch_target(const MCInst &inst, uint64_t address,
                             uint64_t size);
  MachineBasicBlock *get_or_create_mbb_for_address(uint64_t address,
                                                   MachineFunction &mfunc);
  void add_operand_to_mib(MachineInstrBuilder &mib, const MCOperand &mc_op,
                          unsigned op_idx, const MCInstrDesc &desc);
  MachineInstr *create_machine_instr(const translated_inst &tinst,
                                     MachineBasicBlock *mbb);
};

class elf_to_mir_converter {
public:
  elf_to_mir_converter();

  Error process_file(StringRef file_path);
  Error convert_section(StringRef section_name);
  Error write_mir(StringRef output_path);
  void print_mir(raw_ostream &os);

private:
  elf_loader loader;
  std::unique_ptr<elf_disassembler> disassembler;
  std::unique_ptr<translator_t> translator;
};

} // namespace mctomir
