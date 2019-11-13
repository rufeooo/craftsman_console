
#pragma once

#include <stdbool.h>
#include <stdlib.h>

#include "functor.h"

typedef struct {
  const char *name;
  Functor_t fnctor;
} Function_t;

typedef struct {
  const char *name;
  void *address;
  size_t bytes;
} Object_t;

#define MAX_SYMBOLS 32
extern Function_t dlfn_function[MAX_SYMBOLS];
extern int dlfn_used_function;
extern Object_t dlfn_object[MAX_SYMBOLS];
extern int dlfn_used_object;

Function_t *dlfn_get_symbol(const char *name);
Object_t *dlfn_get_object(const char *name);
void dlfn_call(const char *name);
void dlfn_print_functions();
void dlfn_print_objects();

bool dlfn_init(const char *filepath);
void dlfn_shutdown();

bool dlfn_open();
bool dlfn_close();
