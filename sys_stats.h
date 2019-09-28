
#pragma once

typedef struct {
  double moments[3];
} Stats_t;

double sys_statsSamples(Stats_t *accum);
double sys_statsMean(Stats_t *accum);
double sys_statsRsDev(Stats_t *accum);
double sys_statsVariance(Stats_t *accum);

void sys_statsAddSample(Stats_t *accum, double newValue);
