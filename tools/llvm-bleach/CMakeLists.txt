find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
llvm_map_components_to_libnames(llvm_libs ${LLVM_TARGETS_TO_BUILD} support core irreader target codegen passes native
  AllTargetsAsmParsers
  AllTargetsCodeGens
  AllTargetsDescs
  AllTargetsInfos
            analysis
            mirparser
)

add_executable(llvm-bleach llvm-bleach.cpp)
enable_warnings(llvm-bleach)
target_enable_linter(llvm-bleach)
target_link_libraries(llvm-bleach ${llvm_libs})
target_compile_features(llvm-bleach PUBLIC cxx_std_23)
