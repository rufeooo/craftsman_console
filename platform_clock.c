#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "macro.h"
#include "rdtsc.h"

typedef struct {
  // Conversion constants
  uint64_t median_tsc_per_usec;
  uint64_t median_usec_per_tsc;
  // The ideal advacement cadence of tsc_clock
  uint64_t tsc_step;
  // Count the time rhythmic progression was lost
  uint64_t jerk;
  // Rhythmic time progression
  uint64_t tsc_clock;
  // Used for time delta within a single frame
  uint64_t frame_to_frame_tsc;
} TscClock_t;

#define USEC_PER_CLOCK (1000.f / 1000.f / CLOCKS_PER_SEC)
#define CLOCKS_PER_MS (CLOCKS_PER_SEC / 1000)
#define MAX_SAMPLES 10

// clock() will accumulate per thread
// thus restricted to one thread at a time
uint64_t
__tsc_per_usec()
{
  static volatile uint64_t lock_id ALIGNAS(sizeof(uint64_t));
  static volatile uint64_t running_thread ALIGNAS(sizeof(uint64_t));
  uint64_t thread_id = lock_id++;
  while (thread_id != running_thread) {
    struct timespec ts = {0, MAX_SAMPLES * 1000 * 1000};
    nanosleep(&ts, NULL);
  }

  clock_t c = clock();
  clock_t p;
  uint64_t rc = rdtsc();
  uint64_t rp;

  uint64_t tsc_per_usec[MAX_SAMPLES];
  for (int i = 0; i < MAX_SAMPLES; ++i) {
    p = c;
    rp = rc;
    do {
      // clock measures time spent in the program (NB: excludes kernel)
      // a tight loop will push the scheduler to spend time in the application
      c = clock();
    } while (c - p < CLOCKS_PER_MS);
    rc = rdtsc();

    tsc_per_usec[i] = (rc - rp) * ((c - p) * USEC_PER_CLOCK);
  }

  // insertion sort
  for (int i = 1; i < MAX_SAMPLES; ++i) {
    for (int j = i; j > 0; --j) {
      if (tsc_per_usec[j - 1] <= tsc_per_usec[j]) break;
      uint64_t tmp = tsc_per_usec[j - 1];
      tsc_per_usec[j - 1] = tsc_per_usec[j];
      tsc_per_usec[j] = tmp;
    }
  }

  ++running_thread;
  return tsc_per_usec[MAX_SAMPLES / 2];
}

uint64_t
__tscdelta_to_usec(const TscClock_t *clock, uint64_t delta_tsc)
{
  return (delta_tsc * clock->median_usec_per_tsc) >> 33;
}

void
clock_init(uint64_t frame_goal_usec, TscClock_t *out_clock)
{
  const uint64_t tsc_per_usec = __tsc_per_usec();
  out_clock->median_tsc_per_usec = tsc_per_usec;
  printf("clock_init tsc_per_usec %lu \n", tsc_per_usec);

  // Calculate the step
  out_clock->tsc_step = frame_goal_usec * tsc_per_usec;
  out_clock->median_usec_per_tsc = ((uint64_t)1 << 33) / tsc_per_usec;
  // Init to current time
  uint64_t now = rdtsc();
  out_clock->jerk = 0;
  out_clock->tsc_clock = now;
  out_clock->frame_to_frame_tsc = now;
}

uint64_t
clock_delta_usec(const TscClock_t *clock)
{
  return __tscdelta_to_usec(clock, rdtsc() - clock->frame_to_frame_tsc);
}

bool
clock_sync(TscClock_t *clock, uint64_t *optional_sleep_usec)
{
  uint64_t tsc_now = rdtsc();
  uint64_t tsc_previous = clock->tsc_clock;
  uint64_t tsc_next = tsc_previous + clock->tsc_step;

  if (tsc_next - tsc_now < clock->tsc_step) {
    // no-op, busy wait
    // optional sleep time
    *optional_sleep_usec = __tscdelta_to_usec(clock, tsc_next - tsc_now);
    return false;
  } else if (tsc_now - tsc_next <= clock->tsc_step) {
    // frame slightly over goal time
    // trivial advancement continues
    // within tsc_step do not sleep
    clock->tsc_clock = tsc_next;
  } else if (tsc_now - tsc_next > clock->tsc_step) {
    // non-trivial time-jerk, due to hardware clock or massive perf issue
    // do not sleep
    clock->tsc_clock = tsc_now;
    clock->jerk += 1;
  }

  clock->frame_to_frame_tsc = tsc_now;
  return true;
}
