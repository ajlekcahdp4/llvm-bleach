#pragma once

#include <llvm/Object/ObjectFile.h>

#include <cstdint>
#include <string>
#include <vector>

namespace mctomir {

struct function_symbol final {
  std::string name;
  uint64_t start;
  uint64_t end;
};

class file_info final : private std::vector<function_symbol> {
public:
  file_info() = default;

  void add(const function_symbol &s) { vector::push_back(s); }

  using vector::back;
  using vector::begin;
  using vector::empty;
  using vector::end;
  using vector::front;
  using vector::size;
};

file_info load_file_info_from_yaml(std::string yaml);

std::string save_to_yaml(const file_info &);

} // namespace mctomir
