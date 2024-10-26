#include "bleach/lifter/instr-impl.h"

#include <llvm/CodeGen/CommandFlags.h>
#include <llvm/CodeGen/MIRParser/MIRParser.h>
#include <llvm/CodeGen/MachineFunction.h>
#include <llvm/CodeGen/MachineModuleInfo.h>
#include <llvm/CodeGen/MachinePassManager.h>
#include <llvm/CodeGen/TargetPassConfig.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/CGPassBuilderOption.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>

#include <fstream>
#include <iostream>
#include <utility>

using namespace llvm;
cl::OptionCategory options("llvm-bleach options");
namespace bleach {

static cl::opt<std::string> mir_file_name(cl::Positional,
                                          cl::desc("<mir file>"),
                                          cl::cat(options), cl::init(""));

namespace {
Function *func_g = nullptr;

class print_pass : public PassInfoMixin<print_pass> {
public:
  PreservedAnalyses run(Module &m, ModuleAnalysisManager &mam) {
    auto &mmi = mam.getResult<MachineModuleAnalysis>(m).getMMI();
    for (auto &f : m) {
      func_g = &f;
      auto &mf = mmi.getOrCreateMachineFunction(f);
      mf.dump();
    }
    return PreservedAnalyses::all();
  }
};

// remove to get read of unitialized global
codegen::RegisterCodeGenFlags cfg;

} // namespace
} // namespace bleach
using namespace bleach;
auto main(int argc, char **argv) -> int try {
  cl::ParseCommandLineOptions(argc, argv, "llvm-bleach");
  InitializeAllTargetInfos();
  InitializeAllTargets();
  InitializeAllTargetMCs();
  InitializeAllAsmPrinters();
  InitializeAllDisassemblers();
  LLVMContext ctx;
  SMDiagnostic err_mir_parser;
  auto file_copy = mir_file_name.getValue();
  auto mir_parser =
      createMIRParserFromFile(std::move(file_copy), err_mir_parser, ctx);
  if (!mir_parser)
    throw std::runtime_error(err_mir_parser.getMessage().str());
  Triple triple;
  std::unique_ptr<LLVMTargetMachine> target_machine;
  auto set_data_layout = [&](StringRef data_layout_target_triple,
                             StringRef) -> std::optional<std::string> {
    auto target_triple_str = data_layout_target_triple.str();
    if (!target_triple_str.empty())
      target_triple_str = Triple::normalize(target_triple_str);
    triple = Triple(target_triple_str);
    if (triple.getTriple().empty()) {
      triple.setTriple(sys::getDefaultTargetTriple());
    }
    auto tm = codegen::createTargetMachineForTriple(triple.str());
    if (auto err = tm.takeError())
      throw std::runtime_error(toString(std::move(err)));
    target_machine = std::unique_ptr<LLVMTargetMachine>(
        static_cast<LLVMTargetMachine *>(tm->release()));
    return target_machine->createDataLayout().getStringRepresentation();
  };
  auto m = mir_parser->parseIRModule(set_data_layout);
  auto machine_module_info =
      std::make_unique<MachineModuleInfo>(target_machine.get());
  assert(m);
  if (mir_parser->parseMachineFunctions(*m, *machine_module_info))
    throw std::runtime_error("Failed to parse machine functions");
  ModuleAnalysisManager mam;

  ModulePassManager mpm;
  PassBuilder pass_builder;
  // remove to get nullptr dereference
  pass_builder.registerModuleAnalyses(mam);
  mam.registerPass([&] { return PassInstrumentationAnalysis(); });
  mam.registerPass([&] { return MachineModuleAnalysis(*machine_module_info); });
  mpm.addPass(print_pass());
  mpm.run(*m, mam);
  std::ifstream file("build/instrs.yaml");
  std::string yaml(std::istreambuf_iterator<char>{file},
                   std::istreambuf_iterator<char>{});
  assert(file.good());
  auto instrs = lifter::load_from_yaml(std::move(yaml), ctx);
  for (auto &[name, ir_module] : instrs) {
    auto *func = ir_module->getFunction(name);
    assert(func);
    func->dump();
  }
  lifter::save_to_yaml(instrs);
} catch (const std::exception &e) {
  std::cerr << "ERROR: " << e.what() << std::endl;
  exit(EXIT_FAILURE);
}
