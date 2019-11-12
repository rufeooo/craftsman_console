#!/bin/bash
mkdir -p code
if [ -z $@ ]; then
  echo "Exempli Gratia"
  echo "Usage: ./eg.sh [<file> ...]"
  echo "./eg.sh example/djb2_hash.c"
  exit 1
fi
if [ -z $CC ]; then
  CC=`which clang`
fi
if [ -z $CC ]; then
  CC=`which gcc`
fi
echo building with $CC $@
time $CC $@ -g -shared -fpic -nostartfiles -ldl -O0 -o code/feature.so
