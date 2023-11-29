#!/bin/bash

set -ex

BENCHDIR=aes-simple

rm -rf $BENCHDIR
mkdir -p $BENCHDIR
cp *.c $BENCHDIR
cp *.h $BENCHDIR
pushd $BENCHDIR

#HEAP_SIZE=256K # og. 128K
#STACK_SIZE=48K # og. 24K
#COMMON_C_ARGS="-g -specs=htif_nano.specs -Wl,--defsym=__stack_size_min=$STACK_SIZE -Wl,--defsym=__heap_size=$HEAP_SIZE"
COMMON_C_ARGS="-g -specs=htif_nano.specs"
OBJ_C_ARGS="-fno-common -fno-builtin-printf"

riscv64-unknown-elf-gcc $OBJ_C_ARGS $COMMON_C_ARGS -c accellib.c
riscv64-unknown-elf-gcc $OBJ_C_ARGS $COMMON_C_ARGS -c test.c
riscv64-unknown-elf-gcc --static $COMMON_C_ARGS accellib.o test.o -o test.riscv
riscv64-unknown-elf-objdump -S test.riscv > test.dump
popd
