#include "mctomir/mctomir-transform.h"

#include <cstdint>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/CodeGen/MIRParser/MIRParser.h>
#include <llvm/CodeGen/MIRPrinter.h>
#include <llvm/CodeGen/MachineInstrBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/MC/MCInstrAnalysis.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Object/ELFObjectFile.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/SubtargetFeature.h>

#include <iostream>
#include <ranges>
#include <set>

namespace mctomir {
using namespace llvm;
namespace ranges = std::ranges;
namespace views = std::views;
translator_t::translator_t() : tii(nullptr), tri(nullptr), tsi(nullptr) {
  ctx = std::make_unique<LLVMContext>();
}

translator_t::~translator_t() {}

Error translator_t::initialize(StringRef triple_name,
                               SubtargetFeatures features) {
  if (Error err = create_module_and_function(triple_name, features))
    return err;

  if (!mod || !tmachine)
    return createStringError(inconvertibleErrorCode(),
                             "Module or target machine not initialized");
  FunctionType *func_ty = FunctionType::get(Type::getVoidTy(*ctx), false);
  auto *dummy =
      Function::Create(func_ty, Function::ExternalLinkage, "__dummy__", *mod);
  tsi = tmachine->getSubtargetImpl(*dummy);
  dummy->eraseFromParent();
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
  mod->setTargetTriple(the_triple);

  TargetOptions options;
  // TODO(Ilyagavrilin): make TargetMachine initialization simplier or add more
  // features
  tmachine.reset(the_target->createTargetMachine(
      the_triple, "", features.getString(), options, std::nullopt));
  if (!tmachine)
    return createStringError(inconvertibleErrorCode(),
                             "Cannot create target machine for %s",
                             triple_name.data());

  mod->setDataLayout(tmachine->createDataLayout());
  return Error::success();
}

Error translator_t::process_disassembly(SectionRef section,
                                        StringRef contents) {
  if (!tmachine || !tii || !tri || !tsi)
    return createStringError(inconvertibleErrorCode(),
                             "translator not properly initialized");
  uint64_t section_addr = section.getAddress();

  ArrayRef<uint8_t> bytes(reinterpret_cast<const uint8_t *>(contents.data()),
                          contents.size());

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
  std::vector<SymbolRef> functions;
  for (SymbolRef s : section.getObject()->symbols()) {
    auto name = s.getName();
    auto type = s.getType();
    auto addr = s.getValue();
    if (name && type && addr && !name->empty() &&
        (type.get() == SymbolRef::Type::ST_Function ||
         type.get() == SymbolRef::Type::ST_Unknown)) {
      auto *obj = s.getObject();
      auto size = [&] {
        if (isa<ELFObjectFileBase>(obj))
          return ELFSymbolRef(s).getSize();
        throw std::runtime_error("Unsupported object size");
      }();
      funcs.push_back(
          translated_function{{}, {}, *addr, *addr + size, name->str()});
    }
  }
  ranges::sort(funcs, {}, &translated_function::start);

  uint64_t size;
  MCInst inst;
  for (auto &&finfo : funcs) {
    // FIXME: don't always return i64
    finfo.func =
        Function::Create(FunctionType::get(Type::getInt64Ty(*ctx), false),
                         Function::ExternalLinkage, finfo.name, *mod);
    finfo.mfunc = &mmi->getOrCreateMachineFunction(*finfo.func);
    finfo.mfunc->getProperties().reset(
        MachineFunctionProperties::Property::TracksLiveness);
    for (size_t addr = finfo.start; addr < finfo.end;) {
      auto diff = addr - section_addr;
      if (dis_asm->getInstruction(inst, size, bytes.slice(diff), addr,
                                  errs())) {
        finfo.insts.emplace_back(addr, inst);
        addr += size;
      } else {
        errs() << "Illegal instruction at address: 0x" << utohexstr(addr)
               << '\n';
        addr += 1;
      }
    }
  }
  return Error::success();
}

Error translator_t::add_instruction(uint64_t address, const MCInst &inst) {
  return Error::success();
}

Error translator_t::finalize() {
  // TODO(Ilyagavrilin): Rewrite this chain
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

Error translator_t::identify_basic_blocks() {
  address_to_mbb.clear();
  std::set<uint64_t> leaders;
  for (auto &func : funcs) {
    leaders.clear();
    if (!func.insts.empty())
      leaders.insert(func.insts.front().address);
    for (size_t i = 0; i < func.insts.size(); ++i) {
      const translated_inst &tinst = func.insts[i];
      uint64_t current_addr = tinst.address;
      const MCInst &inst = tinst.inst;
      if (mi_analysis->isBranch(inst) || mi_analysis->isReturn(inst)) {
        if (mi_analysis->isBranch(inst)) {
          uint64_t target;
          // TODO(Ilyagavrilin): it breaks on some corner cases, need to
          // redesign
          if (mi_analysis->evaluateBranch(inst, current_addr, 0, target)) {
            leaders.insert(target);
          }
        }
        if (i + 1 < func.insts.size()) {
          leaders.insert(func.insts[i + 1].address);
        }
      }
    }

    for (translated_block *current_block = nullptr; auto &tinst : func.insts) {
      uint64_t current_addr = tinst.address;
      if (leaders.count(current_addr) > 0) {
        if (current_block)
          current_block->end_address = current_addr - 1;

        func.blocks.emplace_back(current_addr);
        current_block = &func.blocks.back();
      }

      if (current_block)
        current_block->instrs.push_back(&tinst);
      else {
        return createStringError(inconvertibleErrorCode(),
                                 "Failed to create basic block");
      }
    }

    if (!func.blocks.empty()) {
      const translated_inst &last_inst = func.insts.back();
      func.blocks.back().end_address =
          last_inst.address + last_inst.inst.size();
    }
  }
  return Error::success();
}

Error translator_t::create_machine_basic_blocks() {
  for (auto &func : funcs) {
    for (auto &block : func.blocks) {
      MachineBasicBlock *mbb = func.mfunc->CreateMachineBasicBlock();
      func.mfunc->push_back(mbb);
      block.mbb = mbb;
      address_to_mbb[block.start_address] = mbb;
    }
  }

  return Error::success();
}

Error translator_t::translate_instructions() {
  if (!tii)
    return createStringError(inconvertibleErrorCode(),
                             "target instruction info is not initialized");

  for (auto &finfo : funcs) {
    for (auto &block : finfo.blocks) {
      for (auto *tinst : block.instrs) {
        MachineInstr *minst = create_machine_instr(*tinst, block.mbb);
        if (!minst)
          return createStringError(inconvertibleErrorCode(),
                                   "Failed to convert instruction at 0x%llx",
                                   tinst->address);
      }
    }
  }

  return Error::success();
}

MachineInstr *translator_t::create_machine_instr(const translated_inst &tinst,
                                                 MachineBasicBlock *mbb) {
  if (!tii || !mbb)
    return nullptr;

  auto &inst = tinst.inst;
  unsigned opcode = inst.getOpcode();
  const MCInstrDesc &desc = tii->get(opcode);
  uint64_t target;
  bool is_branch = mi_analysis->evaluateBranch(inst, 0, 4, target);
  MachineInstrBuilder mib = BuildMI(*mbb, mbb->end(), DebugLoc(), desc);

  for (unsigned i = 0; i < inst.getNumOperands(); ++i) {
    if (is_branch && (i == inst.getNumOperands() - 1) &&
        inst.getOperand(i).isImm())
      break;
    const MCOperand &mc_op = inst.getOperand(i);
    add_operand_to_mib(mib, mc_op, i, desc);
  }

  if (is_branch) {
    // TODO(Ilyagavilin): deduce operands in other way
    if (inst.getNumOperands() > 0 &&
        inst.getOperand(inst.getNumOperands() - 1).isImm()) {
      auto target_it = address_to_mbb.find(target + tinst.address);
      if (target_it != address_to_mbb.end()) {
        if (mi_analysis->isCall(inst))
          mib.addGlobalAddress(&target_it->second->getParent()->getFunction(),
                               0);
        else
          mib.addMBB(target_it->second);
      } else {
        std::string instr;
        raw_string_ostream ss(instr);
        mib->print(ss);
        std::cerr << "Warning: destination not found for instruction:\n\t"
                  << instr << '\n';
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
      uint16_t flags = RegState::Define;

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
  for (auto &finfo : funcs) {
    for (size_t i = 0; i < finfo.blocks.size(); ++i) {
      auto &block = finfo.blocks[i];
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
            MachineBasicBlock *target_mbb =
                get_or_create_mbb_for_address(target, *finfo.mfunc);
            mbb->addSuccessor(target_mbb);
          }
        }
        if (!mi_analysis->isUnconditionalBranch(last_inst) &&
            i + 1 < finfo.blocks.size()) {
          MachineBasicBlock *next_mbb = finfo.blocks[i + 1].mbb;
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
      } else if (i + 1 < finfo.blocks.size()) {
        MachineBasicBlock *next_mbb = finfo.blocks[i + 1].mbb;
        if (next_mbb) {
          mbb->addSuccessor(next_mbb);
        }
      }
    }
  }
  return Error::success();
}

MachineBasicBlock *
translator_t::get_or_create_mbb_for_address(uint64_t address,
                                            MachineFunction &mfunc) {
  auto it = address_to_mbb.find(address);
  if (it != address_to_mbb.end())
    return it->second;

  MachineBasicBlock *mbb = mfunc.CreateMachineBasicBlock();
  mfunc.push_back(mbb);
  address_to_mbb[address] = mbb;

  return mbb;
}

Error translator_t::write_mir(StringRef output_filename) const {
  std::error_code ec;
  raw_fd_ostream os(output_filename, ec);
  if (ec)
    return createFileError(output_filename, errorCodeToError(ec));

  print_mir(os);

  return Error::success();
}

void translator_t::print_mir(raw_ostream &os) const {
  if (!mod)
    throw std::runtime_error("Module does not exist");
  llvm::printMIR(os, *mod);
  for (auto &finfo : funcs) {
    llvm::printMIR(os, *mmi, *finfo.mfunc);
  }
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

Error elf_to_mir_converter::convert_section(StringRef section_name) {
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

  if (Error err = translator->process_disassembly(section, contents))
    return err;

  if (Error err = translator->finalize())
    return err;

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
