bleach_add_llvm_based_tool(
  llvm-bleach
  LLVM_COMPONENTS
  support
  codegen
  mcdisassembler
  mirparser
  analysis
  passes
  AllTargetsInfos
  AllTargetsAsmParsers
  AllTargetsCodeGens
  AllTargetsDescs
  target
  SOURCES
  llvm-bleach.cpp)
target_enable_linter(llvm-bleach)
target_link_libraries(llvm-bleach PUBLIC bleach-passes bleach-headers)
install(TARGETS llvm-bleach DESTINATION ${CMAKE_INSTALL_BINDIR})
