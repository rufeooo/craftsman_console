
#pragma once

#include <stdbool.h>
#include <stdlib.h>

#include "functor.h"

typedef struct {
  const char *name;
  Functor_t fnctor;
} Symbol_t;

typedef struct {
  const char *name;
  void *address;
  size_t bytes;
} Object_t;

static const int MAX_SYMBOLS = 32;
extern Symbol_t dlfnSymbols[MAX_SYMBOLS];
extern int dlfnUsedSymbols;
extern Object_t dlfnObjects[MAX_SYMBOLS];
extern int dlfnUsedObjects;

Symbol_t *dlfn_get_symbol(const char *name);
void *dlfn_get_object(const char *name);
void dlfn_call(const char *name);
void dlfn_print_symbols();

bool dlfn_init(const char *filepath);
void dlfn_shutdown();

bool dlfn_open();
bool dlfn_close();
