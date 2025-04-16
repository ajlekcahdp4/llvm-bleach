#include "mctomir/mctomir-transform.h"

#include <llvm/ADT/StringExtras.h>
#include <llvm/CodeGen/Passes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/MC/MCDisassembler/MCDisassembler.h>
#include <llvm/MC/MCInstPrinter.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Object/SymbolicFile.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/WithColor.h>

using namespace llvm;
using namespace llvm::object;
using namespace mctomir;
//  Command-line options
static cl::opt<std::string>
    input_filename(cl::Positional, cl::desc("<input ELF file>"), cl::Required);
static cl::opt<std::string> output_filename("o", cl::desc("Output MIR file"),
                                            cl::value_desc("filename"),
                                            cl::init(""));
static llvm::cl::opt<bool>
    verbose_output("verbose", llvm::cl::desc("Enable verbose output"),
                   llvm::cl::init(false));
static llvm::cl::alias
    verbose_output_alias("v", llvm::cl::desc("Alias for --verbose"),
                         llvm::cl::aliasopt(verbose_output));

int main(int argc, char **argv) {
  InitLLVM x(argc, argv);
  cl::ParseCommandLineOptions(argc, argv, "LLVM-based ELF-to-MIR converter\n");

  // Initialize all LLVM targets
  InitializeAllTargetInfos();
  InitializeAllTargets();
  InitializeAllTargetMCs();
  InitializeAllAsmParsers();
  InitializeAllAsmPrinters();
  InitializeAllDisassemblers();
  std::string section_name = ".text"; // For now hardcode text section

  elf_to_mir_converter converter;
  if (verbose_output)
    outs() << "Processing file: " << input_filename << "\n";

  if (Error err = converter.process_file(input_filename)) {
    WithColor::error() << "Failed to process file: " << toString(std::move(err))
                       << "\n";
    return 1;
  }

  if (verbose_output)
    outs() << "Converting section: " << section_name << "\n";

  if (Error err = converter.convert_section(section_name)) {
    WithColor::error() << "Failed to convert section: "
                       << toString(std::move(err)) << "\n";
    return 1;
  }

  if (verbose_output)
    outs() << "Writing MIR to: " << output_filename << "\n";
  if (Error err = converter.write_mir(output_filename)) {
    WithColor::error() << "Failed to write MIR: " << toString(std::move(err))
                       << "\n";
    return 1;
  }

  if (verbose_output)
    outs() << "Conversion complete.\n";

  return 0;
}
