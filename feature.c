#include <stdlib.h>

size_t tick;

size_t
game_tick()
{
  ++tick;

  return tick;
}

