
#ifndef __MACRO__H
#define __MACRO__H

#define ARRAY_MEMBER_SIZE(arr) (sizeof(*arr))
#define ARRAY_LENGTH(arr) (sizeof(arr) / sizeof(*arr))
#define ARRAY_LAST_INDEX(arr) (ARRAY_LENGTH(arr) - 1)
#define NULLTERM(arr) arr[sizeof(arr) - 1] = 0
#define MAX(a, b) (a > b ? a : b)
#define MIN(a, b) (a < b ? a : b)
#define CLAMPF(f, min, max) (f < min ? min : f > max ? max : f)
#define CLAMP(d, min, max) (d < min ? min : d > max ? max : d)
#define FREE(a) \
  free(a);      \
  a = NULL;
#define CRASH()                 \
  do {                          \
    *((volatile char *) 0) = 1; \
  } while (true)

#ifndef INLINE
#define INLINE __attribute__((always_inline)) inline
#endif

#ifndef LOCAL
#define LOCAL __attribute__ ((visibility ("hidden"))) 
#endif

#endif
