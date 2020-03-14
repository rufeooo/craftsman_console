#pragma once

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "float.c"
#include "macro.h"
#include "platform_clock.c"
#include "rdtsc.h"

// Memory layout
static TscClock_t tsc_clock;
static uint64_t sleep_usec;
static uint64_t sleep_enable;
static clock_t clock_start;
static uint64_t tsc_start;
static uint32_t frame;
static uint32_t pause_frame;
static uint32_t stall_frame;
static uint32_t sleep_count;
static bool running;
static uint32_t run_count;
static uint32_t input_queue;
static uint32_t input_queue_max;

// Implementation
static INLINE void
loop_one(uint32_t *frame_counter)
{
  *frame_counter += 1;
  while (!clock_sync(&tsc_clock, &sleep_usec)) {
    struct timespec tv = {0, sleep_usec * 1000};
    struct timespec remain;
    while (sleep_enable) {
      ++sleep_count;
      if (!nanosleep(&tv, &remain)) break;
      tv = remain;
    }
  }
}

void
loop_init(uint8_t framerate)
{
  const uint64_t usec_goal = 1000.0L * 1000.0L / framerate;
  clock_init(usec_goal, &tsc_clock);
  clock_start = clock();
  tsc_start = tsc_clock.tsc_clock;

  frame = 0;
  pause_frame = 0;
  stall_frame = 0;
  running = true;
  input_queue = input_queue_max = framerate;
}

void
loop_enable_yield()
{
  sleep_enable = 1;
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
  loop_one(&stall_frame);
}

void
loop_pause()
{
  loop_one(&pause_frame);
}

void
loop_sync()
{
  loop_one(&frame);
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
  printf("[ %d frame ] [ %d pause ] [ %d stall ] [ %d sleep_count ]\n", frame,
         pause_frame, stall_frame, sleep_count);
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

  uint64_t tsc = tsc_clock.tsc_clock;
  uint64_t tscPerMs = tsc_clock.median_tsc_per_usec * 1000;
  uint64_t tscEndEstimate = tsc_start + (tscPerMs * elapsedMs);
  uint64_t tscDrift = MIN(tscEndEstimate - tsc, tsc - tscEndEstimate);
  uint64_t tscDriftMs = tscDrift / tscPerMs;
  printf("[ %" PRIu64 " tsc ] [ %" PRIu64 " tscEnd ] [ %" PRIu64
         " tscDrift ] [ %" PRIu64 " tscPerMs ] [ %" PRIu64 " ms drift ]\n ",
         tsc, tscEndEstimate, tscDrift, tscPerMs, tscDriftMs);

  printf("%" PRIu64 " ms elapsed by clock\n", elapsedMs);
}
