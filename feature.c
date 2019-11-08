#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

size_t tick;
size_t *ref_tick = &tick;
int32_t other;

size_t
game_tick()
{
  ++tick;
  other = -1 * tick;

  return (size_t)((tick % 9) == 0);
}

size_t
game_tick_other()
{
  ++tick;
  other = -1 * tick;

  return tick;
}

void
print_tick(size_t p)
{
  if (p) {
    printf("p: %zu tick: %zu\n", p, tick);
  }
}
