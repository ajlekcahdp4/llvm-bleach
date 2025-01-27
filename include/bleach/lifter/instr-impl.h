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
  std::string stack_pointer;

public:
  instr_impl() = default;
  using vector::at;
  using vector::begin;
  using vector::empty;
  using vector::end;
  using vector::size;
  using vector::operator[];
  using vector::emplace_back;
  using vector::push_back;

  StringRef get_stack_pointer() const { return stack_pointer; }

  void set_stack_pointer(std::string sp) { stack_pointer = std::move(sp); }

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
