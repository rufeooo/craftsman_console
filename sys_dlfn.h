
#pragma once

#include <stdbool.h>
#include <stdlib.h>

#include "functor.h"

typedef struct {
  const char *name;
  Functor_t fnctor;
} Symbol_t;

static const int MAX_SYMBOLS = 32;
extern Symbol_t symbols[MAX_SYMBOLS];
extern int usedSymbols;

Functor_t sys_dlfnGet(const char *name);
void sys_dlfnCall(const char *name);
void sys_dlfnPrintSymbols();

bool sys_dlfnInit(const char *filepath);
void sys_dlfnShutdown();

bool sys_dlfnOpen();
bool sys_dlfnClose();
