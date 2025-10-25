bleach_add_llvm_based_lib(
  mctomir-lifter
  LLVM_COMPONENTS
  support
  mcdisassembler
  target
  mirparser
  irreader
  SOURCES
  elf-loader.cpp
  symbols.cpp
  mctomir-transform.cpp)

target_link_libraries(mctomir-lifter PUBLIC yaml-cpp::yaml-cpp)
target_include_directories(
  mctomir-lifter PUBLIC ${LLVM_BLEACH_INCLUDE_DIR})
