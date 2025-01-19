yaml_dep = dependency('yaml-cpp')

bleach_lifter = library(
  'bleach-lifter',
  ['instr-impl.cpp', 'block-ir-builder.cpp', 'redundant-branch-eraser.cpp'],
  include_directories: bleach_include,
  dependencies: [llvm_dep, yaml_dep],
  install: true,
)
