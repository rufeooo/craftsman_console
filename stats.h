
#pragma once

#include <stdint.h>

typedef struct {
  double moments[3];
  double min;
  double max;
} Stats_t;

void stats_init(Stats_t *accum);
double stats_sample_count(Stats_t *accum);
double stats_mean(Stats_t *accum);
double stats_variance(Stats_t *accum);
double stats_rs_dev(Stats_t *accum);
double stats_min(Stats_t *accum);
double stats_max(Stats_t *accum);

void stats_sample_add(Stats_t *accum, double sample);
