bleach_add_llvm_based_tool(
  mctomir
  LLVM_COMPONENTS
  support
  codegen
  mcdisassembler
  mirparser
  AllTargetsInfos
  AllTargetsAsmParsers
  AllTargetsCodeGens
  AllTargetsDescs
  AllTargetsDisassemblers
  target
  SOURCES
  mctomir.cpp)
target_enable_linter(mctomir)
target_link_libraries(mctomir PUBLIC mctomir-lifter)
target_include_directories(
  mctomir PUBLIC ${LLVM_BLEACH_INCLUDE_DIR})
install(TARGETS mctomir DESTINATION ${CMAKE_INSTALL_BINDIR})
