#!/bin/bash
if [ -z $CC ]; then
  CC=`which clang`
fi
if [ -z $CC ]; then
  CC=`which gcc`
fi
echo building with $CC
time $CC main.c -ldl -lpthread -g -O0 -ffp-contract=off 
