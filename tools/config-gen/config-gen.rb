#!/usr/bin/env ruby
require "yaml"
require "optparse"

module ArchNames
  RV32Prefix = "rv32"
  RV64Prefix = "rv64"
  AArch64Prefix = "aarch64"
end

options = {}
OptionParser
  .new do |opts|
  opts.banner = "Usage: config-gen.rb [options]"
  opts.on("--march ARCH", "Architecture to generate config for") do |a|
    options[:arch] = a
  end

  opts.on("-d", "--template-dir DIR", "Path to templates directory") do |a|
    options[:templ_dir] = a
  end

  opts.on("-t", "--template FILE", "Path to template") do |a|
    options[:templ] = a
  end

  opts.on("-o", "--output FILE", "Output config file") do |a|
    options[:out] = a
  end

  options[:help] = opts.help
end
  .parse!

class Arch
  def initialize(prefix = "rv64im", features = ["i", "m"])
    @prefix = prefix
    @features = features
    if (prefix == ArchNames::RV32Prefix || prefix == ArchNames::RV64Prefix) && !features.include?("m")
      raise "Currently your arch must include M extesnion for RISC-V"
    end
  end

  def get_xlen
    if prefix == ArchNames::RV32Prefix
      return 32
    elsif prefix == ArchNames::RV64Prefix
      return 64
    elsif prefix == ArchNames::AArch64Prefix
      return 64
    else
      raise "Unsupported arch"
    end
  end

  attr_reader :prefix
  attr_reader :features
end

def parse_arch(arch_str = "rv64im")
  # For now support only rv64im and rv32im
  found = ArchNames
    .constants
    .select do |pre|
    arch_str.start_with?(ArchNames.const_get(pre))
  end
    .map { |c| ArchNames.const_get(c) }
  if found.length != 1
    raise(
      "Invalid arch name. Supported prefixes are: \
    #{ArchNames.constants.map { |c| ArchNames.const_get(c) }}"
    )
  end

  prefix = found[0]
  features = arch_str[prefix.length..].chars
  return Arch.new(prefix, features)
end

def get_template_path(arch, templ_dir)
  case arch.prefix
  when ArchNames::RV32Prefix, ArchNames::RV64Prefix
    return File.join(templ_dir, "rv.yaml")
  when ArchNames::AArch64Prefix
    return File.join(templ_dir, "aarch64.yaml")
  else
    raise "Unsupported arch"
  end
end

def get_template(arch, templ_dir)
  path = get_template_path(arch, templ_dir)
  if !File.exist?(path)
    raise "Template directory does not contain \"#{File.basename(path)}\""
  end

  return YAML.load_file(path)
end

def replace_all_expressions(templ, xlen)
  rx = /\\[a-z0-9 \-\+\/\*]+\\/
  match = rx.match(templ)
  while match != nil
    expr = match.to_s
    templ.gsub!(expr, eval(expr[1..-2]).to_s)
    match = rx.match(templ)
  end

  return templ
end

def append_declarations_if_needed(config, func)
  if !config["extern-functions"]
    return func
  end

  func << "\n"
  extern_functions = config["extern-functions"].map do |f|
    [/@[a-zA-Z_0-9]+/.match(f).to_s[1..-1], f]
  end

  extern_functions.select { |f| /@#{f[0]}/.match(func) }.each do |f|
    func << f[1] << "\n"
  end

  func
end

def instantiate_for(templ, arch)
  xlen = arch.get_xlen
  config = {}
  templ.select { |k, v| k != "instructions" }.each do |k, v|
    config[k] = v
  end

  config["instructions"] = templ["instructions"].select { |i, v|
    !v.key?("requires") || v["requires"] == arch.prefix
  }.map do |instr|
    name = instr[0]
    next if instr[1].key?("requires") && instr[1]["requires"] != arch.prefix
    inst = {}
    instr[1].select { |k, v| k != "func" }.each do |k, v|
      inst[k] = v
    end
    if instr[1].key?("func")
      func = instr[1]["func"]
      func = replace_all_expressions(func, xlen)
      func = append_declarations_if_needed(config, func)
      inst["func"] = func
    end
    { name => inst }
  end

  return config
end

if !options[:arch]
  puts("ERROR: no march specified")
  puts(options[:help])
  exit(-1)
end

if !options[:out]
  raise "No output file specified"
end

arch = parse_arch(options[:arch])
if !options[:templ] && !options[:templ_dir]
  raise "Either template directory or template file should be specified"
end

templ = nil
if options[:templ_dir]
  dir_path = File.expand_path(options[:templ_dir])
  raise "Directory #{dir_path} does not exist" unless File.exist?(dir_path)
  templ = get_template(arch, dir_path)
else
  templ_path = File.expand_path(options[:templ])
  raise "Template file does not exist" unless File.exist?(templ_path)
  templ = YAML.load_file(templ_path)
end

config = instantiate_for(templ, arch)
out = options[:out]
if out == "-"
  p(config.to_yaml)
else
  File.open(options[:out], "w") { |f| f.write(config.to_yaml) }
end
