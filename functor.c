
#include "functor.h"

Functor_t
functor_init(FuncPointer fp)
{
  return (Functor_t){ .type = 0, .call = fp };
}

void
functor_param_int(Functor_t *fnctor, int i)
{
  uint32_t type = fnctor->type & 0x00FFFFFF;
  int paramCount = 0;
  while (type) {
    type >>= 8;
    ++paramCount;
  }
  fnctor->param[paramCount].i = i;
  fnctor->type |= 'i' << (8 * paramCount);
}

void
functor_param_pointer(Functor_t *fnctor, void *p)
{
  uint32_t type = fnctor->type & 0x00FFFFFF;
  int paramCount = 0;
  while (type) {
    type >>= 8;
    ++paramCount;
  }
  fnctor->param[paramCount].p = p;
  fnctor->type |= 'p' << (8 * paramCount);
}

#define functor_param(f, X)             \
  _Generic((X), int                     \
           : functor_param_int, default \
           : functor_param_pointer)(f, X)

int
functor_invoke(Functor_t *fnctor)
{
  FuncPointer fp = fnctor->call;
  Param_t u1 = fnctor->param[0];
  Param_t u2 = fnctor->param[1];
  Param_t u3 = fnctor->param[2];

  switch (fnctor->type) {
  case 0:
    return fp();
  case 'i':
    return fp(u1.i);
  case 'p':
    return fp(u1.p);
  case 'ii':
    return fp(u1.i, u2.i);
  case 'ip':
    return fp(u1.i, u2.p);
  case 'pi':
    return fp(u1.p, u2.i);
  case 'pp':
    return fp(u1.p, u2.p);
  case 'iii':
    return fp(u1.i, u2.i, u3.i);
  case 'ipp':
    return fp(u1.i, u2.p, u3.p);
  case 'ipi':
    return fp(u1.i, u2.p, u3.i);
  case 'iip':
    return fp(u1.i, u2.i, u3.p);
  case 'pii':
    return fp(u1.p, u2.i, u3.i);
  case 'pip':
    return fp(u1.p, u2.i, u3.p);
  case 'ppi':
    return fp(u1.p, u2.p, u3.i);
  case 'ppp':
    return fp(u1.p, u2.p, u3.p);
  }

  return -1;
}

