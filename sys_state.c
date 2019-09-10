
#include <string.h>

typedef struct {
  int (*func)();   // C functor? with bound parameters
  System_t *child; // More participants in this system
  System_t *next;  // Next system (TODO: Can we build this from decl of
                   // dependent systems?)
} System_t;

static System_t systemRoot;

System_t *
sys_stateSystemLinkChild(System_t *system, System_t *child)
{
  system->child = child;
  return child;
}

System_t *
sys_stateSystemLinkNext(System_t *system, System_t *next)
{
  system->next = next;
  return next;
}

bool
sys_stateInit(System_t *root)
{
  systemRoot = *root;

  return true;
}

// Preorder traversal: (Root, child, next)
void
sys_stateUpdate(System_t *active)
{
  active->func();
  sys_stateUpdate(active->child);
  sys_stateUpate(active->next);
}

void
sys_stateShutdown()
{
  memset(systemRoot, 0, sizeof(systemRoot));
}
