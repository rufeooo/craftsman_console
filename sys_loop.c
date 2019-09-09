
#include <stdint.h>
#include <stdio.h>
#include <time.h>

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
  uint64_t tscElapsed = rdtsc() - tscStart;

  printf("#%u frame %lu ms duration %lu ms by tsc %lu tscPerMs\n", frame,
         clockElapsed / (CLOCKS_PER_SEC / 1000), tscElapsed / tscPerMs,
         tscPerMs);
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
  uint64_t newTsc;
  for (;;) {
    newTsc = rdtsc();
    if (newTsc - tsc >= tscPerFrame)
      break;
  }
  tsc = newTsc;
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

