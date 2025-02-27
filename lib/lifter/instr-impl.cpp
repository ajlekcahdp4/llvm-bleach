#include "bleach/lifter/instr-impl.h"

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SmallVectorMemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>

#include <yaml-cpp/emitter.h>
#include <yaml-cpp/emittermanip.h>
#include <yaml-cpp/yaml.h>

#include <iostream>

namespace bleach::lifter {
struct normalized_instruction final {
  std::string name;
  std::string function_str;
  normalized_instruction() = default;
  normalized_instruction(const instruction &instr)
      : name(instr.name), function_str([&] {
          std::string str;
          raw_string_ostream os(str);
          if (!instr.ir_module)
            throw std::runtime_error("Attempt to serialize instr with no impl");
          instr.ir_module->print(os, /* AssemblyAnnotationWriter */ nullptr);
          return str;
        }()) {}
};

} // namespace bleach::lifter

namespace YAML {
using namespace bleach;
template <> struct convert<lifter::normalized_instruction> {
  static Node encode(const lifter::normalized_instruction &instr) {
    Node node;
    node[instr.name]["func"] = instr.function_str;
    return node;
  }

  static bool decode(const Node &node, lifter::normalized_instruction &instr) {
    if (node.size() != 1)
      throw std::runtime_error("Instruction entry must have only 1 key");
    for (auto &&inst : node) {
      instr.name = inst.first.as<std::string>();
      instr.function_str = inst.second["func"].as<std::string>();
    }
    return true;
  }
};

template <> struct convert<lifter::instr_impl> {
  static Node encode(const lifter::instr_impl &instrs) {
    Node node;
    for (auto &i : instrs)
      node["instructions"].push_back(lifter::normalized_instruction{i});
    return node;
  }
};
} // namespace YAML

namespace bleach::lifter {

namespace {
instruction denormalize_instruction(const normalized_instruction &norm,
                                    llvm::LLVMContext &ctx) {
  instruction instr;
  instr.name = norm.name;
  llvm::SmallVector<char> ir_buf(norm.function_str.begin(),
                                 norm.function_str.end());
  auto mem_buf = llvm::SmallVectorMemoryBuffer(std::move(ir_buf), "");
  llvm::SMDiagnostic err;
  instr.ir_module = llvm::parseIR(mem_buf, err, ctx);
  if (!instr.ir_module)
    throw std::runtime_error("Failed to parse LLVM IR: " +
                             err.getMessage().str());
  return instr;
}
} // namespace

YAML::Emitter &operator<<(YAML::Emitter &out, const instruction &instr) {
  out << YAML::convert<normalized_instruction>::encode(instr);
  return out;
}

YAML::Emitter &operator<<(YAML::Emitter &out, const instr_impl &instrs) {
  out << YAML::convert<instr_impl>::encode(instrs);
  return out;
}

std::string save_to_yaml(const instr_impl &instrs) {
  YAML::Emitter out;
  out << YAML::BeginMap;
  out << YAML::Key << "instructions";
  out << YAML::Value << YAML::BeginSeq;
  for (auto &i : instrs)
    out << i;
  out << YAML::EndSeq << YAML::EndMap;
  return out.c_str();
}

instr_impl load_from_yaml(std::string yaml, llvm::LLVMContext &ctx) {
  auto instrs_conf = YAML::Load(yaml);
  instr_impl instrs;
  instrs.set_stack_pointer(instrs_conf["stack-pointer"].as<std::string>());
  for (auto &&node : instrs_conf["constant-registers"]) {
    instrs.get_const_regs().push_back(
        {node.first.as<std::string>(), node.second.as<uint64_t>()});
  }
  for (auto &&node : instrs_conf["register-classes"])
    instrs.get_regclasses().push_back(node.as<std::string>());
  for (auto &&node : instrs_conf["instructions"]) {
    auto norm = node.as<normalized_instruction>();
    instrs.push_back(denormalize_instruction(norm, ctx));
  }
  return instrs;
}
} // namespace bleach::lifter
