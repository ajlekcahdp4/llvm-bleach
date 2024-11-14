bleach_add_llvm_based_tool(unit-test-lifter SOURCES lifter.cpp
  LLVM_COMPONENTS
  support
  codegen
  AllTargetsInfos
  AllTargetsAsmParsers
  AllTargetsCodeGens
)
target_link_libraries(unit-test-lifter PRIVATE GTest::gtest_main bleach-lifter)
gtest_discover_tests(unit-test-lifter)
