#!/bin/bash
if [ -z $CC ]; then
  CC=`which clang++`
fi
if [ -z $CC ]; then
  CC=`which g++`
fi
OPT=-fno-omit-frame-pointer
echo building with $CC
time $CC main.cc -std=c++11 -lm -ldl -lpthread -g -O1 $OPT -ffp-contract=off 
