#include <stdio.h>
#include <stdlib.h>

size_t tick;

size_t
game_tick()
{
  ++tick;

  return tick;
}

void
print_tick(size_t p)
{
  if (p)
    printf("tick %zu\n", tick);
}
