
#pragma once

#include <stdint.h>

typedef union {
  int i;
  void *p;
} Param_t;

typedef int (*FuncPointer)();

typedef struct {
  uint32_t type;
  FuncPointer call;
  Param_t param[4];
} Functor_t;

// Methods
Functor_t functor_init(FuncPointer fp);
int functor_invoke(Functor_t *fnctor);

// Generic Parameter binding
#define functor_param(f, X)             \
  _Generic((X), int                     \
           : functor_param_int, default \
           : functor_param_pointer)(f, X)

void functor_param_int(Functor_t *fnctor, int i);
void functor_param_pointer(Functor_t *fnctor, void *p);
