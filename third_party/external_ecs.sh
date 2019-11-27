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
git clone -q git@github.com:AnthonyBrunasso/test_games.git 
git checkout 96210d6
popd
echo building with $CC
time $CC cc_ecs_test.cc --std=c++17 -g -shared -fpic -O0 -o ../code/feature.so
