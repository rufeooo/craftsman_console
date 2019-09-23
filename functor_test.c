#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "functor.h"
#include "macro.h"
#include "rdtsc.h"

size_t
test(size_t a, size_t b, size_t c)
{
  getenv("USERFOO");

  return 0;
}

size_t
test_i(size_t i)
{
  printf("%lu\n", i);
  return 1;
}

size_t
test_ii(size_t i, size_t ii)
{
  printf("%lu %lu\n", i, ii);
  return 2;
}

size_t
test_iii(size_t i, size_t ii, size_t iii)
{
  printf("%lu %lu %lu\n", i, ii, iii);
  return 3;
}

size_t
test_p(const char *p)
{
  printf("%s\n", p);
  return 1;
}

size_t
test_pp(const char *p, const char *pp)
{
  printf("%s %s\n", p, pp);
  return 2;
}

size_t
test_ppp(const char *p, const char *pp, const char *ppp)
{
  printf("%s %s %s\n", p, pp, ppp);
  return 3;
}

size_t
test_pii(const char *p, size_t i, size_t ii)
{
  printf("%s %zu %zu\n", p, i, ii);
  return 3;
}

size_t
test_ppi(const char *p, const char *pp, size_t i)
{
  printf("%s %s %zu\n", p, pp, i);
  return 3;
}

size_t
test_pip(const char *p, size_t i, const char *pp)
{
  printf("%s %zu %s\n", p, i, pp);
  return 3;
}

size_t
test_ipi(size_t i, const char *p, size_t ii)
{
  printf("%zu %s %zu\n", i, p, ii);
  return 3;
}

size_t
test_iip(size_t i, size_t ii, const char *p)
{
  printf("%zu %zu %s\n", i, ii, p);
  return 3;
}

size_t
test_ipp(size_t i, const char *p, const char *pp)
{
  printf("%zu %s %s\n", i, p, pp);
  return 3;
}

size_t
test_ip(size_t i, const char *p)
{
  printf("%zu %s\n", i, p);
  return 2;
}

size_t
test_pi(const char *p, size_t i)
{
  printf("%s %zu\n", p, i);
  return 2;
}

void
test_fnctor_i(Functor_t f)
{
  for (int i = 0; i < 4; ++i) {
    f.param[0].i = i;
    functor_invoke(f);
  }
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      f.param[0].i = i;
      f.param[1].i = j;
      functor_invoke(f);
    }
  }
  for (int i = 0; i < 2; ++i) {
    for (int j = 0; j < 2; ++j) {
      for (int k = 0; k < 2; ++k) {
        f.param[0].i = i;
        f.param[1].i = j;
        f.param[2].i = k;
        functor_invoke(f);
      }
    }
  }
}

void
test_fnctor_p(Functor_t f)
{
  const void *ptr = "foo";
  char *foo[] = {
    "one", "two", "three", "four", "five", "six", "seven", "eight",
  };
  for (int i = 0; i < 4; ++i) {
    f.param[0].p = foo[i];
    functor_invoke(f);
  }
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      f.param[0].p = foo[i];
      f.param[1].p = foo[3 + j];
      functor_invoke(f);
    }
  }
  for (int i = 0; i < 2; ++i) {
    for (int j = 0; j < 2; ++j) {
      for (int k = 0; k < 2; ++k) {
        f.param[0].p = foo[i];
        f.param[1].p = foo[2 + j];
        f.param[2].p = foo[4 + k];
        functor_invoke(f);
      }
    }
  }
}

void
test_fnctor_mix2(Functor_t fip, Functor_t fpi)
{
  char *foo[] = {
    "one", "two", "three", "four", "five", "six", "seven", "eight",
  };
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      fip.param[0].i = i + 1;
      fip.param[1].p = foo[j];
      functor_invoke(fip);
    }
  }
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      fpi.param[0].p = foo[i];
      fpi.param[1].i = j + 1;
      functor_invoke(fpi);
    }
  }
}

void
test_fnctor_mix3()
{
  Functor_t mix3[] = {
    functor_init(test_pii), functor_init(test_ppi), functor_init(test_pip),
    functor_init(test_ipi), functor_init(test_iip), functor_init(test_ipp),
  };

  char *foo[] = {
    "one", "two", "three", "four", "five", "six", "seven", "eight",
  };
  for (int base = 0; base < ARRAY_LENGTH(mix3); ++base) {
    Functor_t f = mix3[base];

    // TODO
  }
}

void
api_call(Functor_t f)
{
  functor_invoke(f);
}

void
manual_call(Functor_t f)
{
  f.call(f.param[0], f.param[1], f.param[2]);
}

int
main(int argc, char **argv)
{
  const unsigned OneMil = 1 * 1000 * 1000;
  uint64_t start = rdtsc();
  Functor_t f = functor_init(test);
  f.param[0].i = 35;
  f.param[2].i = 13;

  // asm review
  api_call(f);
  manual_call(f);

  // Tests
  for (int i = 0; i < OneMil; ++i) {
    functor_invoke(f);
  }
  uint64_t end1 = rdtsc();
  for (int i = 0; i < OneMil; ++i) {
    test(35, 0, 13);
  }
  uint64_t end2 = rdtsc();
  for (int i = 0; i < OneMil; ++i) {
    getenv("USERFOO");
  }
  uint64_t end3 = rdtsc();

  printf("\n%lu functor %lu indirect %lu direct\n", end1 - start, end2 - end1,
         end3 - end2);

  f = functor_init(test_i);
  test_fnctor_i(f);
  puts("--");
  f = functor_init(test_ii);
  test_fnctor_i(f);
  puts("--");
  f = functor_init(test_iii);
  test_fnctor_i(f);
  puts("--end i");
  puts("--begin p");
  f = functor_init(test_p);
  test_fnctor_p(f);
  puts("--");
  f = functor_init(test_pp);
  test_fnctor_p(f);
  puts("--");
  f = functor_init(test_ppp);
  test_fnctor_p(f);
  puts("--end p");

  test_fnctor_mix2(functor_init(test_ip), functor_init(test_pi));
  // TODO: test_fnctor_mix3

  return 0;
}
