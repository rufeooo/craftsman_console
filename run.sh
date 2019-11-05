#!/bin/bash
clang main.c notify.c input.c dlfn.c record.c stats.c functor.c network.c hash.c -ldl -lpthread -g -O0 -ffp-contract=off 
if [ $? -eq 0 ]; then
  LD_LIBRARY_PATH=. ./a.out $@
else
  echo "clang exited:" $?
fi
