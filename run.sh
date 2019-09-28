#!/bin/bash
clang main.c sys_notify.c sys_input.c sys_dlfn.c sys_record.c sys_stats.c functor.c -ldl -g -O0
if [ $? -eq 0 ]; then
  LD_LIBRARY_PATH=. ./a.out
else
  echo "clang exited:" $?
fi
