#include <cstdint>
#include <memory>
#include <unordered_map>

// TODO: allow several instances
std::unordered_map<uint64_t, void *> symtab;

extern "C" {
// addr - address of symbol from source binary
// ptr - blockaddress of lifted llvm ir function entry
void bleach_symtab_add(uint64_t addr, void *ptr) {
  symtab.try_emplace(addr, ptr);
}

void *bleach_symtab_lookup(uint64_t addr) { return symtab.at(addr); }
} // extern "C"
