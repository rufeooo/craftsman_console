
#pragma once

#include <stdint.h>

typedef struct {
  double moments[3];
  double min;
  double max;
} Stats_t;

void sys_statsInit(Stats_t *accum);
double sys_statsSamples(Stats_t *accum);
double sys_statsMean(Stats_t *accum);
double sys_statsRsDev(Stats_t *accum);
double sys_statsVariance(Stats_t *accum);
double sys_statsMin(Stats_t *accum);
double sys_statsMax(Stats_t *accum);

void sys_statsAddSample(Stats_t *accum, double sample);
