
#include <string.h>

typedef struct {
  const char *name; // Common name
  int (*func)();    // C functor? with bound parameters
  System_t *next;
} System_t;

static System_t systemTail = { "tail", NULL, NULL };
static System_t *systemHead = systemTail;

void
sys_stateCreate(const char *name, int (*func)())
{
  System_t *test = calloc(1, sizeof(System_t));
  test->name = strdup(name);
  test->func = func;
  test->next = systemHead;
  systemHead = test;
}

