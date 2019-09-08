
#pragma once

#include <stdbool.h>

bool sys_dlfnInit(const char *filepath);
void sys_dlfnPrintSymbols();
void sys_dlfnCall(const char *name);
bool sys_dlfnOpen();
bool sys_dlfnClose();
void sys_dlfnShutdown();
