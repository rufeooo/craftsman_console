#!/bin/bash
clang main.c notify.c input.c dlfn.c record.c stats.c functor.c hash.c -ldl -g -O0 -ffp-contract=off
if [ $? -eq 0 ]; then
  LD_LIBRARY_PATH=. ./a.out $@
else
  echo "clang exited:" $?
fi
