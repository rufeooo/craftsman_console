
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "macro.h"
#include "rdtsc.h"

// Pure functions
static uint64_t
getTscPerMs()
{
  uint64_t start = rdtsc();
  uint64_t stop = 0;

  clock_t ctime = clock();
  while ((clock() - ctime) * 1000 / CLOCKS_PER_SEC < 1) {
    stop = rdtsc();
  }

  return stop - start;
}

// Memory layout
static clock_t clockStart;
static uint64_t tscStart;
static uint64_t tscPerMs;
static uint64_t tscPerFrame;
static uint64_t tsc;
static uint32_t frame;
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
  printf("#%u frame %lu ms by clock\n", frame, elapsedMs);

  uint64_t tscEndEstimate = tscStart + (tscPerMs * elapsedMs);
  uint64_t tscDrift = tscEndEstimate - tsc;
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

