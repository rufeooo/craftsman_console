
#pragma once

#include <stdbool.h>

typedef int (*SymbolFunc)();

bool sys_dlfnInit(const char *filepath);
void sys_dlfnPrintSymbols();
void sys_dlfnCall(const char *name);
SymbolFunc sys_dlfnGet(const char *name);
bool sys_dlfnOpen();
bool sys_dlfnClose();
void sys_dlfnShutdown();
