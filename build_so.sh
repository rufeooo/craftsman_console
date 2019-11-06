#!/bin/bash
mkdir -p code
if [ -z $CC ]; then
  CC=`which clang`
fi
if [ -z $CC ]; then
  CC=`which gcc`
fi
echo building with $CC
time $CC -g -shared -fpic -nostartfiles -ldl feature.c -O0 -o code/feature.so
