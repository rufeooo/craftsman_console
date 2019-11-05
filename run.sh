#!/bin/bash
if [ -z $CC ]; then
  CC=`which clang`
fi
if [ -z $CC ]; then
  CC=`which gcc`
fi
echo building with $CC
time $CC main.c notify.c input.c dlfn.c record.c stats.c functor.c network.c hash.c -ldl -lpthread -g -O0 -ffp-contract=off 
if [ $? -eq 0 ]; then
  LD_LIBRARY_PATH=. ./a.out $@
else
  echo "$CC exited:" $?
fi
