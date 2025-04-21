#pragma once

#include <llvm/TargetParser/Triple.h>

#include <memory>

namespace mctomir {
using namespace llvm;
class target;
auto select_target(const Triple &triple) -> std::unique_ptr<target>;

} // namespace mctomir
