#include "bleach/lifter/instr-impl.hpp"
#include "bleach/passes/block-ir-builder.hpp"
#include "bleach/passes/redundant-branch-eraser.hpp"

#include "bleach/version.inc"

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
#include <llvm/IR/Verifier.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/FileOutputBuffer.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/CGPassBuilderOption.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/SROA.h>

#include <fstream>
#include <iostream>
#include <utility>

using namespace llvm;
cl::OptionCategory options("llvm-bleach options");
namespace bleach {

static cl::opt<std::string> mir_file_name(cl::Positional,
                                          cl::desc("<mir file>"),
                                          cl::cat(options), cl::init(""));
static cl::opt<std::string> instructions_file(
    "instructions",
    cl::desc("File with (YAML) descriptions of instructions in LLVM IR"),
    cl::cat(options), cl::init(""));

static cl::opt<bool> dump_input_mir("dump-input-mir",
                                    cl::desc("Dump input MIR"),
                                    cl::cat(options));

static cl::opt<std::string>
    dump_input_instructions("dump-input-instructions",
                            cl::desc("Dump input instructions"),
                            cl::value_desc("filename"), cl::cat(options));

static cl::opt<bool>
    no_inline_opt("noinline-instr",
                  cl::desc("Do not inline call to instruction implementations"),
                  cl::cat(options));
static cl::opt<bool>
    assume_function_nop("assume-function-nop",
                        cl::desc("Assume called functions don't change state"),
                        cl::cat(options));
static cl::opt<std::string>
    dump_struct_def_option("state-struct-file",
                           cl::desc("File to dump state struct definition to"),
                           cl::cat(options));

static cl::opt<unsigned>
    stack_size_option("stack-size", cl::desc("Stack size for array (bytes)"),
                      cl::init(8000), cl::cat(options));

namespace {

class print_pass : public PassInfoMixin<print_pass> {
public:
  PreservedAnalyses run(Module &m, ModuleAnalysisManager &mam) {
    auto &mmi = mam.getResult<MachineModuleAnalysis>(m).getMMI();
    for (auto &f : m) {
      auto &mf = mmi.getOrCreateMachineFunction(f);
      mf.print(outs());
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
  cl::AddExtraVersionPrinter([](raw_ostream &os) {
    os << "Bleach version: " LLVM_BLEACH_VERSION_STRING "\n";
  });
  llvm::setBugReportMsg("PLEASE submit a bug report to "
                        "https://github.com/ajlekcahdp4/llvm-bleach/issues and "
                        "include the crash backtrace.\n");
  cl::ParseCommandLineOptions(argc, argv, "llvm-bleach");
  InitLLVM llvm_stacktrace(argc, argv);
  InitializeAllTargetInfos();
  InitializeAllTargets();
  InitializeAllTargetMCs();
  InitializeAllAsmPrinters();
  // InitializeAllDisassemblers();
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
  if (!instructions_file.getNumOccurrences()) {
    std::cerr << "Skipping instructions parsing since no file were provided\n";
    return EXIT_SUCCESS;
  };
  std::ifstream file(instructions_file.getValue());
  std::string yaml(std::istreambuf_iterator<char>{file},
                   std::istreambuf_iterator<char>{});
  if (!file.good())
    throw std::runtime_error("Failed to open file \"" +
                             instructions_file.getValue() + "\"");
  auto instrs = lifter::load_from_yaml(std::move(yaml), ctx);
  if (dump_input_instructions.getNumOccurrences()) {
    auto yaml_str = lifter::save_to_yaml(instrs);
    auto out_buffer =
        FileOutputBuffer::create(dump_input_instructions, yaml_str.size());
    if (auto err = out_buffer.takeError())
      throw std::runtime_error(toString(std::move(err)));
    auto out = std::move(*out_buffer);
    std::copy(yaml_str.begin(), yaml_str.end(), out->getBufferStart());
    if (auto err = out->commit())
      throw std::runtime_error(toString(std::move(err)));
  }
  ModuleAnalysisManager mam;
  FunctionAnalysisManager fam;
  LoopAnalysisManager lam;
  CGSCCAnalysisManager cgam;

  ModulePassManager mpm;
  PassBuilder pass_builder;
  // remove to get nullptr dereference
  pass_builder.registerModuleAnalyses(mam);
  pass_builder.registerFunctionAnalyses(fam);
  pass_builder.registerLoopAnalyses(lam);
  pass_builder.registerCGSCCAnalyses(cgam);
  pass_builder.crossRegisterProxies(lam, fam, cgam, mam);
  mam.registerPass([&] { return PassInstrumentationAnalysis(); });
  mam.registerPass([&] { return MachineModuleAnalysis(*machine_module_info); });
  if (dump_input_mir)
    mpm.addPass(print_pass());
  mpm.addPass(bleach::lifter::block_ir_builder_pass(
      instrs, assume_function_nop.getValue(), dump_struct_def_option.getValue(),
      stack_size_option.getValue()));
  mpm.addPass(createModuleToFunctionPassAdaptor(
      bleach::lifter::redundant_branch_eraser()));
  if (!no_inline_opt)
    mpm.addPass(createModuleToPostOrderCGSCCPassAdaptor(InlinerPass()));
  mpm.addPass(
      createModuleToFunctionPassAdaptor(SROAPass(SROAOptions::PreserveCFG)));
  mpm.addPass(VerifierPass(false));
  mpm.run(*m, mam);
  m->print(outs(), nullptr);
  mam.clear();
  fam.clear();
  cgam.clear();
  lam.clear();
} catch (const std::exception &e) {
  std::cerr << "ERROR: " << e.what() << std::endl;
  exit(EXIT_FAILURE);
}
