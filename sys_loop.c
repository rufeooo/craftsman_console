
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "macro.h"
#include "rdtsc.h"

// Pure functions
int
tscSort(const void *lhs, const void *rhs)
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
getTscPerMs()
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

  qsort(tscDelta, DELTA_COUNT, sizeof(tscDelta[0]), tscSort);

  return tscDelta[DELTA_COUNT / 2];
}

// Memory layout
static clock_t clockStart;
static uint64_t tscStart;
static uint64_t tscPerMs;
static uint64_t tscPerFrame;
static uint64_t tsc;
static uint32_t frame;
static uint32_t pauseFrame;
static bool running;

// Implementation
void
sys_loopInit(uint8_t framerate)
{
  tscPerMs = getTscPerMs();
  tscPerFrame = 1000 / framerate * tscPerMs;
  clockStart = clock();
  tsc = tscStart = rdtsc();
  frame = 0;
  running = true;
}

void
sys_loopPrintStatus()
{
  clock_t clockElapsed = clock() - clockStart;
  uint64_t elapsedMs = clockElapsed / (CLOCKS_PER_SEC / 1000);
  printf("#%u frame #%u pauseFrame %lu ms by clock\n", frame, pauseFrame,
         elapsedMs);

  uint64_t tscEndEstimate = tscStart + (tscPerMs * elapsedMs);
  uint64_t tscDrift = MIN(tscEndEstimate - tsc, tsc - tscEndEstimate);
  uint64_t tscDriftMs = tscDrift / tscPerMs;
  printf("%lu tsc %lu tscEnd %lu tscDrift %lu tscPerMs %lu ms of drift\n ",
         tsc, tscEndEstimate, tscDrift, tscPerMs, tscDriftMs);
}

bool
sys_loopRun()
{
  return running;
}

void
sys_loopPause()
{
  ++pauseFrame;
  for (;;) {
    uint64_t now = rdtsc();
    if (now - tsc >= tscPerFrame) {
      tsc = MIN(tsc + tscPerFrame, now);
      break;
    }
  }
}

void
sys_loopSync()
{
  ++frame;
  for (;;) {
    uint64_t now = rdtsc();
    if (now - tsc >= tscPerFrame) {
      tsc = MIN(tsc + tscPerFrame, now);
      break;
    }
  }
}

void
sys_loopHalt()
{
  running = false;
}

void
sys_loopShutdown()
{
}

