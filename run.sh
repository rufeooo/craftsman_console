#!/bin/bash
bash ./cc_build.sh
if [ $? -eq 0 ]; then
  LD_LIBRARY_PATH=. ./a.out $@
else
  echo "$CC exited:" $?
fi
