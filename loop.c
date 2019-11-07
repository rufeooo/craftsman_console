
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "float.h"
#include "macro.h"
#include "rdtsc.h"

// Pure functions
int
tsc_order_double(const void *lhs, const void *rhs)
{
  double lhv = *(double *) lhs;
  double rhv = *(double *) rhs;

  if (lhv < rhv)
    return -1;
  else if (lhv > rhv)
    return 1;
  else
    return 0;
}

#define TSC_DELTA_COUNT 15
static uint64_t
calc_tsc_per_us()
{
  uint64_t tscDelta[TSC_DELTA_COUNT + 1] = { rdtsc() };
  clock_t clockDelta[TSC_DELTA_COUNT + 1] = { clock() };
  for (int i = 1; i < TSC_DELTA_COUNT + 1; ++i) {
    clock_t now;
    do {
      clockDelta[i] = clock();
      tscDelta[i] = rdtsc();
    } while ((clockDelta[i] - clockDelta[i - 1]) * 1000 / CLOCKS_PER_SEC < 1);
  }

  double tscPerSec[TSC_DELTA_COUNT + 1];
  for (int i = 0; i < TSC_DELTA_COUNT; ++i) {
    uint64_t clock_delta = (clockDelta[i + 1] - clockDelta[i]);
    uint64_t tsc_delta = tscDelta[i + 1] - tscDelta[i];
    tscPerSec[i] = to_double(CLOCKS_PER_SEC) * to_double(tsc_delta)
                   / to_double(clock_delta);
  }

  qsort(tscPerSec, TSC_DELTA_COUNT, ARRAY_MEMBER_SIZE(tscPerSec),
        tsc_order_double);

  const double USEC_PER_SEC = 1000 * 1000;
  return double_round_uint64(tscPerSec[TSC_DELTA_COUNT / 2] / USEC_PER_SEC);
}

// Memory layout
static clock_t clock_start;
static uint64_t tsc_start;
static uint64_t tsc_per_ms;
static uint64_t *tsc_per_frame;
static uint64_t tsc_per_fast_frame;
static uint64_t tsc_per_stable_frame;
static uint64_t tsc;
static uint32_t frame;
static uint32_t pause_frame;
static uint32_t stall_frame;
static bool running;
static uint32_t run_count;
static uint32_t input_queue;
static uint32_t input_queue_max;

// Implementation
void
loop_init(uint8_t framerate)
{
  if (!tsc_per_ms) {
    tsc_per_ms = calc_tsc_per_us() * 1000;
    tsc_per_fast_frame = 100 / framerate * tsc_per_ms;
    tsc_per_stable_frame = 1000 / framerate * tsc_per_ms;
    tsc_per_frame = &tsc_per_stable_frame;
    clock_start = clock();
    tsc_start = rdtsc();
  }
  tsc = rdtsc();
  frame = 0;
  pause_frame = 0;
  stall_frame = 0;
  running = true;
  input_queue = input_queue_max = framerate;
}

bool
loop_run()
{
  return running;
}

uint32_t
loop_frame()
{
  return frame;
}

void
loop_stall()
{
  const uint64_t tscPer = *tsc_per_frame;
  ++stall_frame;
  for (;;) {
    uint64_t now = rdtsc();
    if (now - tsc >= tscPer) {
      tsc = MIN(tsc + tscPer, now);
      break;
    }
  }
}

void
loop_pause()
{
  const uint64_t tscPer = *tsc_per_frame;
  ++pause_frame;
  for (;;) {
    uint64_t now = rdtsc();
    if (now - tsc >= tscPer) {
      tsc = MIN(tsc + tscPer, now);
      break;
    }
  }
}

void
loop_sync()
{
  const uint64_t tscPer = *tsc_per_frame;
  ++frame;
  for (;;) {
    uint64_t now = rdtsc();
    if (now - tsc >= tscPer) {
      tsc = MIN(tsc + tscPer, now);
      break;
    }
  }
}

uint32_t
loop_input_frame()
{
  return frame + pause_frame;
}

uint32_t
loop_input_queue()
{
  return input_queue;
}

uint32_t
loop_input_queue_max()
{
  return input_queue_max;
}

uint32_t
loop_write_frame()
{
  return frame + pause_frame + input_queue;
}

bool
loop_fast_forward(uint32_t max_queued_commands)
{
  bool fast_forward = max_queued_commands > input_queue_max;
  tsc_per_frame = fast_forward ? &tsc_per_fast_frame : &tsc_per_stable_frame;
  return fast_forward;
}

void
loop_halt()
{
  running = false;
}

void
loop_shutdown()
{
  ++run_count;
}

void
loop_print_frame()
{
  printf("[ %d frame ] [ %d pause ] [ %d stall ] \n", frame, pause_frame,
         stall_frame);
}

void
loop_print_input()
{
  printf("[ %d input_queue ] [ %d loop_write_frame] \n", input_queue,
         loop_write_frame());
}

void
loop_print_status()
{
  clock_t clockElapsed = clock() - clock_start;
  uint64_t elapsedMs = clockElapsed / (CLOCKS_PER_SEC / 1000);
  printf("--%u loop %s--\n", run_count, running ? "Running" : "Terminating");
  loop_print_frame();

  uint64_t tscEndEstimate = tsc_start + (tsc_per_ms * elapsedMs);
  uint64_t tscDrift = MIN(tscEndEstimate - tsc, tsc - tscEndEstimate);
  uint64_t tscDriftMs = tscDrift / tsc_per_ms;
  printf("[ %" PRIu64 " tsc ] [ %" PRIu64 " tscEnd ] [ %" PRIu64
         " tscDrift ] [ %" PRIu64 " tsc_per_ms ] [ %" PRIu64 " ms drift ]\n ",
         tsc, tscEndEstimate, tscDrift, tsc_per_ms, tscDriftMs);

  printf("%" PRIu64 " ms elapsed by clock\n", elapsedMs);
}

