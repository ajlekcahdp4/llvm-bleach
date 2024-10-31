find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

include(GNUInstallDirs)

bleach_add_llvm_based_tool(
  llvm-bleach
  LLVM_COMPONENTS
  support
  codegen
  mcdisassembler
  mirparser
  AllTargetsInfos
  AllTargetsAsmParsers
  AllTargetsCodeGens
  AllTargetsDescs
  target
  SOURCES
  llvm-bleach.cpp)
target_enable_linter(llvm-bleach)
target_link_libraries(llvm-bleach PUBLIC ${llvm_libs} bleach-lifter)
target_compile_features(llvm-bleach PUBLIC cxx_std_23)
set_target_properties(llvm-bleach PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                             "${CMAKE_BINARY_DIR}/bin")

install(TARGETS llvm-bleach DESTINATION ${CMAKE_INSTALL_BINDIR})
