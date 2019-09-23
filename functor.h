
#pragma once

#include <stddef.h>
#include <stdint.h>

typedef union {
  size_t i;
  void *p;
} Param_t;

typedef size_t (*FuncPointer)();

typedef struct {
  FuncPointer call;
  Param_t param[3];
} Functor_t;

// Methods
__attribute__((always_inline)) inline Functor_t
functor_init(FuncPointer fp)
{
  return (Functor_t){ .call = fp };
}
__attribute__((always_inline)) inline size_t
functor_invoke(Functor_t *fnctor)
{
  return fnctor->call(fnctor->param[0], fnctor->param[1], fnctor->param[2]);
}

