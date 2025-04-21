#include "mctomir/mctomir-transform.h"
#include "mctomir/target/target-selector.hpp"
#include "mctomir/target/target.hpp"

#include <llvm/ADT/STLExtras.h>
#include <llvm/CodeGen/MIRParser/MIRParser.h>
#include <llvm/CodeGen/MIRPrinter.h>
#include <llvm/CodeGen/MachineInstrBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/MC/MCInstrAnalysis.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/SubtargetFeature.h>

#include <set>

namespace mctomir {
using namespace llvm;

translator_t::translator_t()
    : tii(nullptr), tri(nullptr), tsi(nullptr), func(nullptr), mfunc(nullptr) {
  ctx = std::make_unique<LLVMContext>();
}

translator_t::~translator_t() {
  for (auto &block : blocks) {
    block.instrs.clear();
  }
}

Error translator_t::initialize(StringRef triple_name,
                               SubtargetFeatures features) {
  if (Error err = create_module_and_function(triple_name, features))
    return err;

  if (Error err = create_machine_function())
    return err;

  return Error::success();
}

Error translator_t::create_module_and_function(StringRef triple_name,
                                               SubtargetFeatures features) {
  Triple the_triple(Triple::normalize(triple_name));

  std::string err_msg;
  const Target *the_target = TargetRegistry::lookupTarget(triple_name, err_msg);
  if (!the_target)
    return createStringError(inconvertibleErrorCode(),
                             "Cannot find target for triple %s: %s",
                             triple_name.data(), err_msg.c_str());

  mod = std::make_unique<Module>("DisassembledModule", *ctx);
  mod->setTargetTriple(triple_name);

  TargetOptions options;
  // TODO(Ilyagavrilin): make TargetMachine initialization simplier or add more
  // features
  std::unique_ptr<TargetMachine> generic_tmachine(
      the_target->createTargetMachine(triple_name, "", features.getString(),
                                      options, std::nullopt));
  if (!generic_tmachine)
    return createStringError(inconvertibleErrorCode(),
                             "Cannot create target machine for %s",
                             triple_name.data());

  tmachine.reset(static_cast<LLVMTargetMachine *>(generic_tmachine.release()));

  mod->setDataLayout(tmachine->createDataLayout());
  // TODO(Ilyagavrilin): add function name resolving, now just a template
  FunctionType *func_ty = FunctionType::get(Type::getVoidTy(*ctx), false);
  func = Function::Create(func_ty, Function::ExternalLinkage,
                          "disassembled_function", *mod);

  return Error::success();
}

Error translator_t::create_machine_function() {
  if (!func || !tmachine)
    return createStringError(inconvertibleErrorCode(),
                             "Module or target machine not initialized");

  tsi = tmachine->getSubtargetImpl(*func);
  if (!tsi)
    return createStringError(inconvertibleErrorCode(),
                             "Failed to get subtarget info");

  tii = tsi->getInstrInfo();
  if (!tii)
    return createStringError(inconvertibleErrorCode(),
                             "Failed to get instruction info");

  tri = tsi->getRegisterInfo();
  if (!tri)
    return createStringError(inconvertibleErrorCode(),
                             "Failed to get register info");

  mmi = std::make_unique<MachineModuleInfo>(tmachine.get());
  mmi->initialize();

  mfunc = &mmi->getOrCreateMachineFunction(*func);

  return Error::success();
}

Error translator_t::process_disassembly(ArrayRef<uint8_t> bytes,
                                        uint64_t base_address) {
  if (!tmachine || !mfunc || !tii || !tri || !tsi)
    return createStringError(inconvertibleErrorCode(),
                             "translator not properly initialized");

  // mia used to identify branches, calls, etc.
  std::unique_ptr<MCInstrAnalysis> mia(
      tmachine->getTarget().createMCInstrAnalysis(tmachine->getMCInstrInfo()));
  if (!mia)
    return createStringError(inconvertibleErrorCode(),
                             "Failed to create MCInstrAnalysis");
  mi_analysis = std::move(mia);

  if (!mcctx) {
    mcctx = std::make_unique<MCContext>(
        Triple(tmachine->getTargetTriple()), tmachine->getMCAsmInfo(),
        tmachine->getMCRegisterInfo(), tmachine->getMCSubtargetInfo());
  }

  std::unique_ptr<const MCDisassembler> dis_asm(
      tmachine->getTarget().createMCDisassembler(*tsi, *mcctx));
  if (!dis_asm)
    return createStringError(inconvertibleErrorCode(),
                             "Failed to create disassembler");

  uint64_t address = base_address;
  uint64_t size;
  MCInst inst;

  for (size_t index = 0; index < bytes.size();) {
    if (dis_asm->getInstruction(inst, size, bytes.slice(index), address,
                                nulls())) {
      if (Error err = add_instruction(address, inst))
        return err;

      index += size;
      address += size;
    } else {
      outs() << "Undefined ruction!!!" << "\n";
      ++index;
      ++address;
    }
  }

  return Error::success();
}

Error translator_t::add_instruction(uint64_t address, const MCInst &inst) {
  instructions.emplace_back(address, inst);
  return Error::success();
}

Error translator_t::finalize() {
  if (instructions.empty())
    return createStringError(inconvertibleErrorCode(),
                             "No instructions to translate");

  // TODO(Ilyagavrilin): Rewrite this chain
  if (Error err = create_initial_basic_block())
    return err;
  if (Error err = identify_basic_blocks())
    return err;
  if (Error err = create_machine_basic_blocks())
    return err;
  if (Error err = translate_instructions())
    return err;
  if (Error err = link_machine_basic_blocks())
    return err;

  return Error::success();
}

Error translator_t::create_initial_basic_block() {
  MachineBasicBlock *entry_mbb = mfunc->CreateMachineBasicBlock();
  mfunc->push_back(entry_mbb);
  // TODO(Ilyagavrilin): check cases when we really need to sort
  llvm::stable_sort(instructions, [](auto &lhs, auto &rhs) {
    return lhs.address < rhs.address;
  });

  initial_mbb = entry_mbb;
  return Error::success();
}

Error translator_t::identify_basic_blocks() {
  blocks.clear();
  address_to_mbb.clear();
  std::set<uint64_t> leaders;
  if (!instructions.empty())
    leaders.insert(instructions.front().address);
  for (size_t i = 0; i < instructions.size(); ++i) {
    const translated_inst &tinst = instructions[i];
    uint64_t current_addr = tinst.address;
    const MCInst &inst = tinst.inst;

    if (mi_analysis->isBranch(inst) || mi_analysis->isCall(inst) ||
        mi_analysis->isReturn(inst)) {
      if (mi_analysis->isBranch(inst)) {
        uint64_t target;
        // TODO(Ilyagavrilin): it breaks on some corner cases, need to redesign
        if (mi_analysis->evaluateBranch(inst, current_addr, 0, target)) {
          leaders.insert(target);
        }
      }
      if (i + 1 < instructions.size()) {
        leaders.insert(instructions[i + 1].address);
      }
    }
  }

  block_info *current_block = nullptr;

  for (auto &tinst : instructions) {
    uint64_t current_addr = tinst.address;
    if (leaders.count(current_addr) > 0) {
      if (current_block)
        current_block->end_address = current_addr - 1;

      blocks.emplace_back(current_addr);
      current_block = &blocks.back();
    }

    if (current_block)
      current_block->instrs.push_back(&tinst);
    else {
      return createStringError(inconvertibleErrorCode(),
                               "Failed to create basic block");
    }
  }

  if (current_block && !instructions.empty()) {
    const translated_inst &last_inst = instructions.back();
    current_block->end_address = last_inst.address + last_inst.inst.size();
  }

  return Error::success();
}

Error translator_t::create_machine_basic_blocks() {
  if (!mfunc)
    return createStringError(inconvertibleErrorCode(),
                             "Machine function not initialized");

  for (auto &block : blocks) {
    MachineBasicBlock *mbb = mfunc->CreateMachineBasicBlock();
    mfunc->push_back(mbb);
    block.mbb = mbb;
    address_to_mbb[block.start_address] = mbb;
  }

  return Error::success();
}

Error translator_t::translate_instructions() {
  if (!tii || !mfunc)
    return createStringError(
        inconvertibleErrorCode(),
        "target instruction info or machine function not initialized");

  for (auto &block : blocks) {
    for (auto *tinst : block.instrs) {
      MachineInstr *minst = create_machine_instr(tinst->inst, block.mbb);
      if (!minst)
        return createStringError(inconvertibleErrorCode(),
                                 "Failed to convert instruction at 0x%llx",
                                 tinst->address);

      tinst->minst = minst;
    }
  }

  return Error::success();
}

MachineInstr *translator_t::create_machine_instr(const MCInst &inst,
                                                 MachineBasicBlock *mbb) {
  if (!tii || !mfunc || !mbb)
    return nullptr;

  unsigned opcode = inst.getOpcode();
  const MCInstrDesc &desc = tii->get(opcode);

  MachineInstrBuilder mib = BuildMI(*mbb, mbb->end(), DebugLoc(), desc);

  for (unsigned i = 0; i < inst.getNumOperands(); ++i) {
    const MCOperand &mc_op = inst.getOperand(i);
    add_operand_to_mib(mib, mc_op, i, desc);
  }

  if (mi_analysis->isBranch(inst)) {
    // TODO(Ilyagavilin): deduce operands in other way
    if (inst.getNumOperands() > 0 &&
        inst.getOperand(inst.getNumOperands() - 1).isImm()) {
      uint64_t target;
      if (mi_analysis->evaluateBranch(inst, 0, 4, target)) {
        auto target_it = address_to_mbb.find(target);
        if (target_it != address_to_mbb.end()) {
          mib.addMBB(target_it->second);
        }
      }
    }
  }

  return mib.getInstr();
}

void translator_t::add_operand_to_mib(MachineInstrBuilder &mib,
                                      const MCOperand &mc_op, unsigned op_idx,
                                      const MCInstrDesc &desc) {
  if (mc_op.isReg()) {
    unsigned reg = mc_op.getReg();

    bool is_def = op_idx < desc.getNumDefs();

    if (is_def) {
      // Add additional flags for better output
      uint16_t flags = RegState::Define | RegState::Renamable;

      mib.addReg(reg, flags);
    } else {
      uint16_t flags = 0;
      // demo of killed instruction marking
      // TODO(Ilyagavrilin): make killed operands analysis
      if (op_idx == desc.getNumOperands() - 1) {
        flags |= RegState::Kill;
      }

      mib.addReg(reg, flags);
    }
  } else if (mc_op.isImm()) {
    mib.addImm(mc_op.getImm());
  } else
    llvm_unreachable("Unsupported operand");
}

// TODO(Ilyagavrilin): Change linkage logic
Error translator_t::link_machine_basic_blocks() {
  if (!mfunc || blocks.empty())
    return createStringError(
        inconvertibleErrorCode(),
        "Machine function not initialized or no basic blocks");

  for (size_t i = 0; i < blocks.size(); ++i) {
    auto &block = blocks[i];
    MachineBasicBlock *mbb = block.mbb;

    if (block.instrs.empty())
      continue;

    auto *last_t_inst = block.instrs.back();
    const MCInst &last_inst = last_t_inst->inst;

    if (mi_analysis->isBranch(last_inst)) {
      uint64_t target;
      if (mi_analysis->evaluateBranch(last_inst, last_t_inst->address, 0,
                                      target)) {
        auto target_it = address_to_mbb.find(target);
        if (target_it != address_to_mbb.end()) {
          mbb->addSuccessor(target_it->second);
        } else {
          MachineBasicBlock *target_mbb = get_or_create_mbb_for_address(target);
          mbb->addSuccessor(target_mbb);
        }
      }
      if (!mi_analysis->isUnconditionalBranch(last_inst) &&
          i + 1 < blocks.size()) {
        MachineBasicBlock *next_mbb = blocks[i + 1].mbb;
        if (next_mbb) {
          bool has_successor = false;
          for (MachineBasicBlock *succ : mbb->successors()) {
            if (succ == next_mbb) {
              has_successor = true;
              break;
            }
          }
          if (!has_successor) {
            mbb->addSuccessor(next_mbb);
          }
        }
      }
    } else if (mi_analysis->isReturn(last_inst)) {
    } else if (i + 1 < blocks.size()) {
      MachineBasicBlock *next_mbb = blocks[i + 1].mbb;
      if (next_mbb) {
        mbb->addSuccessor(next_mbb);
      }
    }
  }

  return Error::success();
}

MachineBasicBlock *
translator_t::get_or_create_mbb_for_address(uint64_t address) {
  auto it = address_to_mbb.find(address);
  if (it != address_to_mbb.end())
    return it->second;

  MachineBasicBlock *mbb = mfunc->CreateMachineBasicBlock();
  mfunc->push_back(mbb);
  address_to_mbb[address] = mbb;

  return mbb;
}

target &translator_t::get_or_create_target() {
  if (!tgt)
    tgt = select_target(tmachine->getTargetTriple());
  return *tgt;
}

void translator_t::make_returns() {
  assert(mfunc);
  auto &tgt = get_or_create_target();
  for (auto &mblock : drop_begin(*mfunc)) {
    auto &last = mblock.back();
    if (last.isReturn())
      continue;
    if (tgt.is_return(last)) {
      last.eraseFromParent();
      tgt.insert_return(mblock, mblock.end(), *tmachine);
    }
  }
}

Error translator_t::write_mir(StringRef output_filename) const {
  if (!mfunc)
    return createStringError(inconvertibleErrorCode(),
                             "Machine function not initialized");

  std::error_code ec;
  raw_fd_ostream os(output_filename, ec);
  if (ec)
    return createFileError(output_filename, errorCodeToError(ec));

  print_mir(os);

  return Error::success();
}

void translator_t::print_mir(raw_ostream &os) const {
  if (!mfunc || !mod)
    return;

  llvm::printMIR(os, *mfunc);
}

elf_to_mir_converter::elf_to_mir_converter() {
  translator = std::make_unique<translator_t>();
}
// Hide main for now
Error elf_to_mir_converter::process_file(StringRef file_path) {
  if (Error err = loader.load_file(file_path))
    return err;

  disassembler = std::make_unique<elf_disassembler>(loader);
  if (Error err = disassembler->initialize())
    return err;
  Expected<Triple> triple_or_err = loader.get_triple();
  if (!triple_or_err)
    return triple_or_err.takeError();

  Expected<SubtargetFeatures> features_or_err =
      disassembler->get_disasm_features();
  if (!features_or_err)
    return features_or_err.takeError();
  SubtargetFeatures features = features_or_err.get();
  if (Error err = translator->initialize(triple_or_err.get().str(), features))
    return err;

  return Error::success();
}

Error elf_to_mir_converter::convert_section(StringRef section_name,
                                            bool match_returns) {
  if (!disassembler || !translator)
    return createStringError(inconvertibleErrorCode(),
                             "disassembler or translator not initialized");

  SectionRef section;
  if (Error err = loader.get_section_by_name(section_name, section))
    return err;

  Expected<StringRef> contents_or_err = loader.get_section_contents(section);
  if (!contents_or_err)
    return contents_or_err.takeError();

  StringRef contents = contents_or_err.get();
  uint64_t section_addr = section.getAddress();

  ArrayRef<uint8_t> bytes(reinterpret_cast<const uint8_t *>(contents.data()),
                          contents.size());

  if (Error err = translator->process_disassembly(bytes, section_addr))
    return err;

  if (Error err = translator->finalize())
    return err;
  if (match_returns)
    translator->make_returns();

  return Error::success();
}

Error elf_to_mir_converter::write_mir(StringRef output_path) {
  if (!translator)
    return createStringError(inconvertibleErrorCode(),
                             "translator not initialized");

  return translator->write_mir(output_path);
}

void elf_to_mir_converter::print_mir(raw_ostream &os) {
  if (translator)
    translator->print_mir(os);
}
} // namespace mctomir
