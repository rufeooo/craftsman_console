#!/bin/bash
clang main.c -ldl -g -O0
LD_LIBRARY_PATH=. ./a.out
