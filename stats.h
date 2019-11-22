
#pragma once

typedef struct {
  double moments[3];
  double min;
  double max;
} Stats_t;

void stats_init(Stats_t *accum);
void stats_init_array(unsigned n, Stats_t accum[static n]);
void stats_sample_add(double sample, Stats_t *accum);

double stats_sample_count(const Stats_t *accum);
double stats_mean(const Stats_t *accum);
double stats_variance(const Stats_t *accum);
double stats_rs_dev(const Stats_t *accum);
double stats_min(const Stats_t *accum);
double stats_max(const Stats_t *accum);

