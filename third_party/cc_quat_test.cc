
#include <stdio.h>

#include "../float.c"
#include "submodule/test_games/math/quat.h"

using namespace math;

static Quat<double> qt;
static Vec3<double> axis;

extern "C" {

size_t set_axis(ApiDouble_t x, ApiDouble_t y, ApiDouble_t z) {
  Vec3<double> new_axis(x.value, y.value, z.value);
  axis = new_axis;

  return 0;
}

size_t rotate(ApiDouble_t degrees) {
  qt.Set(degrees.value,  axis);
  printf("%f %f %f %f \n", qt[0], qt[1], qt[2], qt[3]);

  return 0;
}

}
