#!/bin/bash
mkdir -p code
if [ -z $CC ] || [ "$CC" == "clang" ]; then
  CC=`which clang++`
fi
if [ -z $CC ] || [ "$CC" == "gcc" ]; then
  CC=`which g++`
fi
echo building with $CC
time $CC -g -shared -fpic -ldl ~/projects/external/test_games/math/cc_quat_test.cc -O0 -o code/feature.so
