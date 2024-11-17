#include "bleach/lifter/redundant-branch-eraser.hpp"

#include <llvm/IR/Analysis.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>

#include <ranges>

namespace bleach::lifter {
using namespace llvm;
namespace ranges = std::ranges;
PreservedAnalyses redundant_branch_eraser::run(Module &m,
                                               ModuleAnalysisManager &) {
  for (auto &f : m) {
    for (auto &bb : f) {
      auto branch = ranges::find_if(bb, [](auto &inst) {
        auto *branch = dyn_cast<BranchInst>(&inst);
        return branch && branch->isConditional();
      });
      if (branch != bb.end())
        bb.erase(std::next(branch), bb.end());
    }
  }
  return PreservedAnalyses::none();
}
} // namespace bleach::lifter
