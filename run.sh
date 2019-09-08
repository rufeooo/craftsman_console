#!/bin/bash
clang main.c sys_notify.c sys_input.c sys_dlfn.c -ldl -g -O0
LD_LIBRARY_PATH=. ./a.out
