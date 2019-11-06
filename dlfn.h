
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

#define MAX_SYMBOLS 32
extern Symbol_t dlfn_symbols[MAX_SYMBOLS];
extern int dlfn_used_symbols;
extern Object_t dlfn_objects[MAX_SYMBOLS];
extern int dlfn_used_objects;

Symbol_t *dlfn_get_symbol(const char *name);
Object_t *dlfn_get_object(const char *name);
void dlfn_call(const char *name);
void dlfn_print_symbols();
void dlfn_print_objects();

bool dlfn_init(const char *filepath);
void dlfn_shutdown();

bool dlfn_open();
bool dlfn_close();
