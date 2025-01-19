project(
  'llvm-bleach',
  ['c', 'cpp'],
  version: '0.1.0',
  default_options: ['warning_level=3', 'cpp_std=c++23'],
)

llvm_dep = dependency('llvm', version: ['>=19'], static: false,
  method: 'config-tool', include_type: 'system', required: true)
bleach_include = include_directories('include')

subdir('lib')
subdir('tools')

if get_option('tests')
  subdir('test')
endif

