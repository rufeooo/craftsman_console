#pragma once

#include <stddef.h>
#include <stdint.h>

#include "macro.h"

typedef union {
  size_t i;
  void *p;
  double d;
  const void *cp;
} Param_t;

typedef size_t (*FuncPointer)();
#define PARAM_COUNT 3

typedef struct {
  FuncPointer call;
  Param_t param[PARAM_COUNT];
} Functor_t;

static INLINE Functor_t
functor_init(FuncPointer fp)
{
  return (Functor_t){ .call = fp };
}

static INLINE size_t
functor_invoke(Functor_t fnctor)
{
  return fnctor.call(fnctor.param[0], fnctor.param[1], fnctor.param[2]);
}

_Static_assert(sizeof(size_t) == sizeof(void *),
               "Param_t must used bindings of the same width.");
