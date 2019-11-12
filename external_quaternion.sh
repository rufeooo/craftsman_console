#!/bin/bash
mkdir -p submodule
if [ -z $CC ] || [ "$CC" == "clang" ]; then
  CC=`which clang++`
fi
if [ -z $CC ] || [ "$CC" == "gcc" ]; then
  CC=`which g++`
fi
echo cloning
pushd submodule
git clone git@github.com:rufeooo/test_games.git 
popd
echo building with $CC
time $CC -g -shared -fpic -ldl submodule/test_games/math/cc_quat_test.cc -O0 -o code/feature.so
