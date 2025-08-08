#include "bleach/compiler/compiler.hpp"

#include <clang/Basic/DiagnosticOptions.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Driver/Compilation.h>
#include <clang/Driver/Driver.h>
#include <clang/Driver/Tool.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/FormatVariadic.h>

#include <vector>

namespace bleach::cc {
using namespace llvm;
std::unique_ptr<Module> compile_to_llvm(std::string_view source,
                                        std::string_view source_file,
                                        std::span<const char *> extra_args) {
  clang::CompilerInstance cc;
  cc.createDiagnostics();
  std::vector<const char *> args = {"clang", "-xc++", "-"};
  args.insert(args.end(), extra_args.begin(), extra_args.end());
  auto invocation = std::make_shared<clang::CompilerInvocation>();
  clang::CompilerInvocation::CreateFromArgs(*invocation, args,
                                            cc.getDiagnostics());
  cc.setInvocation(invocation);
  auto source_ref = StringRef(source.data(), source.size());
  auto buffer = MemoryBuffer::getMemBufferCopy(source_ref, source_file);
  cc.getPreprocessorOpts().addRemappedFile(source_file, buffer.get());
  auto action = std::make_unique<clang::EmitLLVMOnlyAction>();
  if (!cc.ExecuteAction(*action))
    throw std::runtime_error(formatv(
        "Failed to run clang to compile source from \"{}\"", source_file));
  return action->takeModule();
}
} // namespace bleach::cc
