#pragma once

#include <llvm/IR/Module.h>

#include <memory>
#include <span>
#include <string_view>

namespace bleach::cc {
std::unique_ptr<llvm::Module>
compile_to_llvm(std::string_view source,
                std::string_view source_file = "bleach_lookup.cpp",
                std::span<const char *> extra_args = {});
}
