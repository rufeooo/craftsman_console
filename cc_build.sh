#!/bin/bash
if [ -z $CC ]; then
  CC=`which clang`
fi
if [ -z $CC ]; then
  CC=`which gcc`
fi
OPT=-fno-omit-frame-pointer
echo building with $CC
time $CC main.c -std=gnu11 -lm -ldl -lpthread -g -O1 $OPT -ffp-contract=off 
