find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
llvm_map_components_to_libnames(llvm_libs support core irreader)

add_library(bleach-lifter instr-impl.cpp)
enable_warnings(bleach-lifter)
target_enable_linter(bleach-lifter)
target_compile_features(bleach-lifter PUBLIC cxx_std_23)
