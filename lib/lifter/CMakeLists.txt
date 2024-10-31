find_package(yaml-cpp REQUIRED CONFIG)

bleach_add_llvm_based_lib(
  bleach-lifter
  LLVM_COMPONENTS
  support
  mcdisassembler
  target
  mirparser
  irreader
  SOURCES
  instr-impl.cpp
  block-ir-builder.cpp)

target_enable_linter(bleach-lifter)
target_include_directories(bleach-lifter PUBLIC ${LLVM_BLEACH_INCLUDE_DIR})
target_link_libraries(bleach-lifter PUBLIC yaml-cpp::yaml-cpp)
target_compile_features(bleach-lifter PUBLIC cxx_std_23)
target_compile_options(bleach-lifter PUBLIC "-O0")
