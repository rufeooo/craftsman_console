
#pragma once

#ifndef INLINE
#define INLINE __attribute__((always_inline)) inline
#endif

#if defined(__i386__) || defined(__x86_64__)

static INLINE uint64_t
rdtsc(void)
{
  return __builtin_ia32_rdtsc();
}

#elif defined(__aarch64__)

static INLINE uint64_t
rdtsc(void)
{
  uint64_t virtual_timer_value;
  asm volatile("mrs %0, cntvct_el0" : "=r"(virtual_timer_value));
  return virtual_timer_value;
}

#else

#pragma error("rdtsc not defined")

#endif
