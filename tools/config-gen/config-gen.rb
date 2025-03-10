#!/usr/bin/env ruby
require "yaml"
require "optparse"

module ArchNames
  RV32Prefix = "rv32"
  RV64Prefix = "rv64"
end

options = {}
OptionParser.new do |opts|
  opts.banner = "Usage: config-gen.rb [options]"
  opts.on("--march ARCH", "Architecture to generate config for") do
    |a| options[:arch] = a end
  opts.on("-d", "--template-dir DIR", "Path to templates directory") do
    |a| options[:templ_dir] = a end
  opts.on("-o", "--output FILE", "Output config file") do
    |a| options[:out] = a end
  options[:help] = opts.help
end.parse!

class Arch
  def initialize(prefix = "rv64im", features = ["i", "m"])
    @prefix = prefix
    @features = features
    if !features.include? "m"
      raise "Currently your arch must include M extesnion for RISC-V"
    end
  end

  def get_xlen()
    if prefix == ArchNames::RV32Prefix
      return 32
    elsif prefix == ArchNames::RV64Prefix
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
  found = ArchNames.constants.select do |pre|
    arch_str.start_with? ArchNames.const_get(pre)
  end.map { |c| ArchNames.const_get(c) }
  if found.length != 1
    raise "Invalid arch name. Supported prefixes are: \
    #{ArchNames.constants.map { |c| ArchNames.const_get c }}"
  end
  prefix = found[0]
  features = arch_str[prefix.length..].chars
  return Arch.new(prefix, features)
end

def get_template_path(arch, templ_dir)
  case arch.prefix
  when ArchNames::RV32Prefix, ArchNames::RV64Prefix
    return File.join(templ_dir, "rv.yaml")
  else
    raise "Unsupported arch"
  end
end

def get_template(arch, templ_dir)
  path = get_template_path(arch, templ_dir)
  if !File.exist? path
    raise "Template directory does not contain \"#{File.basename path}\""
  end
  return YAML.load_file(path)
end

def replace_all_expressions(templ, xlen)
  rx = /\\[a-z0-9 \-\+\/\*]+\\/
  match = rx.match(templ)
  while match != nil
    expr = match.to_s
    templ.gsub! expr, eval(expr[1..-2]).to_s
    match = rx.match(templ)
  end
  return templ
end

def instantiate_for(templ, arch)
  xlen = arch.get_xlen();
  config = {}
  templ.select {|k, v| k != "instructions"}.each do |k, v|
    config[k] = v
  end
  config["instructions"] = templ["instructions"].map do |instr|
    name = instr[0]
    next if instr[1].key?("requires") && instr[1]["requires"] != arch.prefix
    func = instr[1]["func"]
    func = replace_all_expressions(func, xlen)
    { name => {"func" => func}}
  end
  return config
end

if !options[:arch]
  puts "ERROR: no march specified"
  puts options[:help]
  exit(-1)
end
if !options[:templ_dir]
  raise "No template dir specified"
end
if !options[:out]
  raise "No output file specified"
end

arch = parse_arch options[:arch]
dir_path = File.expand_path(options[:templ_dir])
raise "Directory #{dir_path} does not exist" unless File.exist? dir_path

templ = get_template(arch, dir_path)
config = instantiate_for(templ, arch)
out = options[:out]
if out == "-"
  p config.to_yaml
else
  File.open(options[:out], 'w') { |f| f.write config.to_yaml }
end


