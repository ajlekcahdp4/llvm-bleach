#pragma once

#include <llvm/CodeGen/MachineFunction.h>
#include <llvm/IR/Instruction.h>

#include <unordered_map>
#include <vector>

namespace bleach::lifter {
using namespace llvm;

struct instruction final {
  std::string name;
  std::unique_ptr<Module> ir_module;
};

// @class instr_impl
// @brief map from instr opcode to llvm ir impl
class instr_impl final : private std::vector<instruction> {
public:
  instr_impl() = default;
  using vector::at;
  using vector::begin;
  using vector::end;
  using vector::operator[];
  using vector::emplace_back;
  using vector::push_back;

  auto &get(std::string_view name) const & {
    auto found = std::find_if(begin(), end(),
                              [name](auto &pair) { return pair.name == name; });
    if (found == end())
      throw std::runtime_error("Unknown instruction \"" + std::string(name) +
                               "\"");
    assert(found->ir_module);
    return *found->ir_module;
  }
};

instr_impl load_from_yaml(std::string, LLVMContext &ctx);

std::string save_to_yaml(const instr_impl &);

} // namespace bleach::lifter
