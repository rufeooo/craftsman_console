
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "macro.h"
#include "rdtsc.h"

// Pure functions
int
tsc_order(const void *lhs, const void *rhs)
{
  uint64_t lhv = *(uint64_t *) lhs;
  uint64_t rhv = *(uint64_t *) rhs;

  if (lhv < rhv)
    return -1;
  else if (lhv > rhv)
    return 1;
  else
    return 0;
}

static uint64_t
tsc_per_ms()
{
  const int DELTA_COUNT = 15;
  uint64_t tscDelta[DELTA_COUNT + 1] = { rdtsc() };
  clock_t clockDelta[DELTA_COUNT + 1] = { clock() };
  for (int i = 1; i < DELTA_COUNT + 1; ++i) {
    clock_t now;
    do {
      clockDelta[i] = clock();
      tscDelta[i] = rdtsc();
    } while ((clockDelta[i] - clockDelta[i - 1]) * 1000 / CLOCKS_PER_SEC < 1);
  }

  for (int i = 0; i < DELTA_COUNT; ++i) {
    clockDelta[i] = (clockDelta[i + 1] - clockDelta[i]);
    tscDelta[i] = tscDelta[i + 1] - tscDelta[i];
  }

  qsort(tscDelta, DELTA_COUNT, sizeof(tscDelta[0]), tsc_order);

  return tscDelta[DELTA_COUNT / 2];
}

// Memory layout
static clock_t clockStart;
static uint64_t tscStart;
static uint64_t tscPerMs;
static uint64_t *tscPerFrame;
static uint64_t tscPerFastFrame;
static uint64_t tscPerStableFrame;
static uint64_t tsc;
static uint32_t frame;
static uint32_t pauseFrame;
static uint32_t stallFrame;
static bool running;
static uint32_t runCount;

// Implementation
void
loop_init(uint8_t framerate)
{
  if (!tscPerMs) {
    tscPerMs = tsc_per_ms();
    tscPerFastFrame = 100 / framerate * tscPerMs;
    tscPerStableFrame = 1000 / framerate * tscPerMs;
    tscPerFrame = &tscPerStableFrame;
    clockStart = clock();
    tscStart = rdtsc();
  }
  tsc = rdtsc();
  frame = 0;
  pauseFrame = 0;
  stallFrame = 0;
  running = true;
}

void
loop_print_status()
{
  printf("--%u loop %s--\n", runCount, running ? "Running" : "Terminating");
  clock_t clockElapsed = clock() - clockStart;
  uint64_t elapsedMs = clockElapsed / (CLOCKS_PER_SEC / 1000);
  printf("#%u frame #%u pauseFrame #%u stallFrame %" PRIu64 " ms by clock\n",
         frame, pauseFrame, stallFrame, elapsedMs);

  uint64_t tscEndEstimate = tscStart + (tscPerMs * elapsedMs);
  uint64_t tscDrift = MIN(tscEndEstimate - tsc, tsc - tscEndEstimate);
  uint64_t tscDriftMs = tscDrift / tscPerMs;
  printf("%" PRIu64 " tsc %" PRIu64 " tscEnd %" PRIu64 " tscDrift %" PRIu64
         " tscPerMs %" PRIu64 " ms of drift\n ",
         tsc, tscEndEstimate, tscDrift, tscPerMs, tscDriftMs);
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
  const uint64_t tscPer = *tscPerFrame;
  ++stallFrame;
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
  const uint64_t tscPer = *tscPerFrame;
  ++pauseFrame;
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
  const uint64_t tscPer = *tscPerFrame;
  ++frame;
  for (;;) {
    uint64_t now = rdtsc();
    if (now - tsc >= tscPer) {
      tsc = MIN(tsc + tscPer, now);
      break;
    }
  }
}

void
loop_adjustment(size_t pending_bytes)
{
  if (pending_bytes > 40)
    tscPerFrame = &tscPerFastFrame;
  else
    tscPerFrame = &tscPerStableFrame;
}

void
loop_halt()
{
  running = false;
}

void
loop_shutdown()
{
  ++runCount;
}

