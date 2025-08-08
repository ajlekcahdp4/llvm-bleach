#include <cstdint>
#include <unordered_map>
#include <memory>

struct bleach_symtab final {
  std::unordered_map<uint64_t, void *> map;
};

// TODO: allow several instances
bleach_symtab symtab;

extern "C" {
// addr - address of symbol from source binary
// ptr - blockaddress of lifted llvm ir function entry
void bleach_symtab_add(uint64_t addr, void *ptr) {
  symtab.map.try_emplace(addr, ptr);
}

void *bleach_symtab_lookup(uint64_t addr) {
  return symtab.map.at(addr);
}
}
