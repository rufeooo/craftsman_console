#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "dlfn.h"
#include "float.h"
#include "functor.h"
#include "hash.h"
#include "rdtsc.h"
#include "stats.h"

#define MAX_FUNC 128

size_t result[MAX_SYMBOLS];
static Functor_t apply_func[MAX_FUNC];
static size_t used_apply_func;
static Functor_t result_func[MAX_FUNC];
static size_t used_result_func;

size_t
increment(size_t *val)
{
  *val += 1;
  return 1;
}

size_t
left_shift(size_t *val)
{
  *val <<= 1;
  return 1;
}

size_t
decrement(size_t *val)
{
  *val -= 1;
  return 1;
}

size_t
copy(const size_t *src, size_t *dst)
{
  *dst = *src;
  return 1;
}

size_t
print_result(const Symbol_t *sym, size_t r)
{
  printf("0x%zX %s 0x%zX 0x%zX 0x%zX\n", r, sym->name, sym->fnctor.param[0].i,
         sym->fnctor.param[1].i, sym->fnctor.param[2].i);
  return 0;
}

int
tokenize(size_t len, char *input, size_t token_count,
         char *token[token_count])
{
  char *iter = input;
  char *end = input + len;
  token[0] = input;
  int tc = 1;

  for (; iter < end && tc < token_count; ++iter) {
    if (*iter == ' ') {
      *iter = 0;
      token[tc++] = iter + 1;
    }
  }

  return tc;
}

bool
add_apply_func(Functor_t fnctor)
{
  if (used_apply_func >= MAX_FUNC)
    return false;

  apply_func[used_apply_func] = fnctor;
  ++used_apply_func;
  return true;
}

bool
add_result_func(Functor_t fnctor)
{
  if (used_result_func >= MAX_FUNC)
    return false;

  result_func[used_result_func] = fnctor;
  ++used_result_func;
  return true;
}

void
apply_param(const char *pstr, Param_t *p)
{
  uint64_t val = strtol(pstr, 0, 0);
  if (val) {
    printf("pstr %s val %" PRIu64 "\n", pstr, val);
    p->i = val;
  } else if (pstr[0] == '+') {
    printf("pstr %s increment\n", pstr);
    Functor_t fnctor = { .call = increment, .param[0].p = &p->i };
    add_apply_func(fnctor);
  } else if (pstr[0] == '-') {
    printf("pstr %s decrement\n", pstr);
    Functor_t fnctor = { .call = decrement, .param[0].p = &p->i };
    add_apply_func(fnctor);
  } else if (pstr[0] == '<') {
    printf("pstr %s left_shift\n", pstr);
    Functor_t fnctor = { .call = left_shift, .param[0].p = &p->i };
    add_apply_func(fnctor);
  } else if (pstr[0] == '@') {
    const Symbol_t *sym = dlfn_get_symbol(&pstr[1]);
    if (sym) {
      uint32_t sym_offset = (sym - dlfn_symbols);
      Functor_t fnctor = { .call = copy,
                           .param[0].p = &result[sym_offset],
                           .param[1].p = &p->i };
      add_result_func(fnctor);
      printf("pstr result of %s [offset %u]\n", &pstr[1], sym_offset);

    } else {
      printf("sym not found %s\n", &pstr[1]);
    }
  } else { // TODO
    printf("parameter unhandled %s\n", pstr);
  }
}

void
execute_benchmark()
{
  Stats_t aggregate;
  stats_init(&aggregate);

  const int magnitudes = 10;
  int calls = 1;
  for (int h = 0; h < magnitudes; ++h) {
    Stats_t perfStats[MAX_SYMBOLS];
    for (int i = 0; i < MAX_SYMBOLS; ++i) {
      stats_init(&perfStats[i]);
    }

    for (int i = 0; i < dlfn_used_symbols; ++i) {
      double sum = 0;
      for (int j = 0; j < calls; ++j) {
        uint64_t startCall = rdtsc();
        functor_invoke(dlfn_symbols[i].fnctor);
        uint64_t endCall = rdtsc();
        double duration = to_double(endCall - startCall);
        stats_sample_add(&perfStats[i], duration);
        sum += duration;
      }
      stats_sample_add(&aggregate, sum);
    }

    printf("--per 10e%d\n", h);
    for (int i = 0; i < dlfn_used_symbols; ++i) {
      printf("%-20s\t(%5.2e, %5.2e) range\t%5.2e mean ± %4.02f%%\t\n",
             dlfn_symbols[i].name, stats_min(&perfStats[i]),
             stats_max(&perfStats[i]), stats_mean(&perfStats[i]),
             100.0 * stats_rs_dev(&perfStats[i]));
    }
    puts("");

    if (aggregate.max > 10000000.0)
      break;

    calls *= 10;
  }

  puts("benchmark threshold reached.");
}

// simulation <until frame>
#define SIMULATION_MAX UINT32_MAX
uint32_t
execute_simulation(size_t len, char *input)
{
  const unsigned TOKEN_COUNT = 2;
  char *token[TOKEN_COUNT];
  int tc = tokenize(len, input, TOKEN_COUNT, token);

  if (tc == 1)
    return SIMULATION_MAX;

  uint64_t val = strtol(token[1], 0, 0);
  return val;
}

// apply <fn> <p1> <p2> <p3> <...>
void
execute_apply(size_t len, char *input)
{
  const unsigned TOKEN_COUNT = 6;
  char *token[TOKEN_COUNT];
  int tc = tokenize(len, input, TOKEN_COUNT, token);

  if (tc < 2) {
    puts("Usage: apply <func> <param1> <param2> <param3>");
    return;
  }

  int matched = 0;
  for (int fi = 0; fi < dlfn_used_symbols; ++fi) {
    const char *filter = token[1][0] == '*' ? dlfn_symbols[fi].name : token[1];

    if (strcmp(filter, dlfn_symbols[fi].name))
      continue;

    ++matched;
    for (int pi = 0; pi < PARAM_COUNT; ++pi) {
      int ti = 2 + pi;
      if (ti >= tc)
        break;
      apply_param(token[ti], &dlfn_symbols[fi].fnctor.param[pi]);
    }
  }

  if (!matched) {
    printf("Failure to apply: function not found (%s).\n", token[1]);
    return;
  }
}

void
execute_object(size_t len, char *input)
{
  const unsigned TOKEN_COUNT = 2;
  char *token[TOKEN_COUNT];
  int tc = tokenize(len, input, TOKEN_COUNT, token);

  if (tc < 2) {
    puts("Usage: object <name>");
    return;
  }
  void *addr = dlfn_get_object(token[1]);
  if (!addr) {
    printf("%s not found.\n", token[1]);
    return;
  }
  long *vp = addr;
  printf("%s: %p value %lu\n", token[1], addr, *vp);
}

void
execute_hash(size_t len, char *input)
{
  uint64_t hash_seed = 5381;
  for (int i = 0; i < dlfn_used_objects; ++i) {
    hash_seed =
      memhash_cont(hash_seed, dlfn_objects[i].address, dlfn_objects[i].bytes);
  }
  printf("Hashval %lu\n", hash_seed);
}

