
#include "functor.h"

_Static_assert(sizeof(size_t) == sizeof(void *),
               "Param_t must used bindings of the same width.");

extern Functor_t functor_init(FuncPointer fp);
extern size_t functor_invoke(Functor_t fnctor);

