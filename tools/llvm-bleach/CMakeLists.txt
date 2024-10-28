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
include(GNUInstallDirs)
add_executable(llvm-bleach llvm-bleach.cpp)
enable_warnings(llvm-bleach)
target_enable_linter(llvm-bleach)
target_include_directories(llvm-bleach PUBLIC ${LLVM_BLEACH_INCLUDE_DIR})
target_link_libraries(llvm-bleach  PUBLIC ${llvm_libs} bleach-lifter)
target_compile_features(llvm-bleach PUBLIC cxx_std_23)
set_target_properties(llvm-bleach PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

install(TARGETS llvm-bleach DESTINATION ${CMAKE_INSTALL_BINDIR})
