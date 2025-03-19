#!/usr/bin/env bash
layout=$1
valuegram=$2
seed=$3
bld_dir=$4/$seed
src_dir=$5
bleach_cfg=$6
num_instrs=$7
runner=$8
target_clang=$9
mir_file=$bld_dir/snippy.$seed.mir
rv_asm_file=$bld_dir/snippy.S

set -x
set -e
mkdir -p $bld_dir

$SNIPPY_PATH/llvm-snippy $layout --dump-mir=$mir_file --seed $seed \
  --initial-regs-yaml=$valuegram -o $bld_dir/snippy.elf -num-instrs $num_instrs
sed -i '/isCalleeSavedInfoValid/d' $mir_file
sed -i '/varArgsSaveSize/d' $mir_file
sed -i '/varArgsFrameIndex/d' $mir_file
sed -i 's/\(x[13567][0-9]\? = \([A-Z]\{3,\}\).*\$\)x2\(,\|$\)/\1x11\3/g' $mir_file
sed -i 's/\(x[13567][0-9]\? = \([A-Z]\{3,\}\).*\$\)x2\(,\|$\)/\1x11\3/g' $mir_file
sed -i 's/\(x[1-9][0-9]\? = \(OR\).*\$\)x[12]\(,\|$\)/\1x11\3/g' $mir_file
sed -i 's/\(x[1-9][0-9]\? = \(OR\).*\$\)x[12]\(,\|$\)/\1x11\3/g' $mir_file
sed -i 's/\(x2[0-9] = \([A-Z]\{3,\}\).*\$\)x2\(,\|$\)/\1x11\3/g' $mir_file
sed -i 's/\(x2[0-9] = \([A-Z]\{3,\}\).*\$\)x2\(,\|$\)/\1x11\3/g' $mir_file
sed -i 's/\([A-Z]\{3,\}.*\$\)x1\(,\|$\)/\1x10\2/g' $mir_file
sed -i 's/\([A-Z]\{3,\}.*\$\)x1\(,\|$\)/\1x10\2/g' $mir_file
sed -i 's/x1 = \([A-Z]\{3,\}\)/x10 = \1/g' $mir_file
sed -i '/\$x1 = ADDI \$x0, 0/d' $mir_file

riscv64-unknown-linux-gnu-llc $mir_file -o $rv_asm_file -march=riscv64 -mattr=+m,+f,+d,-c -O0
riscv64-unknown-linux-musl-gcc -c $src_dir/main-rv64.c -o $bld_dir/main-rv64.o -march=rv64imfd
riscv64-unknown-linux-gnu-clang -c $src_dir/print.S -o $bld_dir/print.o -march=rv64imfd
riscv64-unknown-linux-musl-gcc $bld_dir/main-rv64.o $bld_dir/print.o $rv_asm_file -o $bld_dir/rv.out -march=rv64imfd --static -O0
qemu-riscv64 $bld_dir/rv.out > $bld_dir/rv.log
$BLEACH_PATH/llvm-bleach $mir_file --instructions=$bleach_cfg \
  -march=riscv64 --assume-function-nop > $bld_dir/lifted.ll
sed -i 's/define void \@print/define weak void @print/g' $bld_dir/lifted.ll
$target_clang $src_dir/main.c $bld_dir/lifted.ll -o $bld_dir/lifted.out -O3 --static -march=rv64imfd
$runner $bld_dir/lifted.out > $bld_dir/lifted.log
diff $bld_dir/rv.log $bld_dir/lifted.log
rm -rd $bld_dir
