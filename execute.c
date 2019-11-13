#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "dlfn.h"
#include "float.h"
#include "functor.h"
#include "global.c"
#include "hash.h"
#include "macro.h"
#include "rdtsc.h"
#include "stats.h"

#define MAX_FUNC 128

static size_t result[MAX_SYMBOLS];
static uint64_t hash_result[MAX_FUNC];
static Functor_t apply_func[MAX_FUNC];
static int symbol_load[MAX_FUNC][PARAM_COUNT];
static size_t used_apply_func;
static __uint128_t apply_func_condition;
static Functor_t result_func[MAX_FUNC];
static size_t used_result_func;

size_t
increment(size_t *val)
{
  *val += 1;
  return 1;
}

size_t
increment_fp(double *val)
{
  *val += 1.0;
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
decrement_fp(double *val)
{
  *val -= 1.0;
  return 1;
}

size_t
copy(Param_t *dst, const Param_t *src)
{
  printf("Copy %p <- %p\n", dst, src);
  *dst = *src;
  printf("Copied %lu <- %lu\n", dst->i, src->i);
  return 1;
}

size_t
fn_filter(size_t result_index, size_t apply_index)
{
  __uint128_t bit = (1 << apply_index);
  if (result[result_index]) {
    apply_func_condition |= bit;
  } else {
    apply_func_condition &= ~bit;
  }

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

int
add_apply_func(Functor_t fnctor)
{
  if (used_apply_func >= MAX_FUNC)
    return -1;

  int idx = used_apply_func;
  apply_func[idx] = fnctor;
  ++used_apply_func;
  return idx;
}

int
add_result_func(Functor_t fnctor)
{
  if (used_result_func >= MAX_FUNC)
    return -1;

  int idx = used_result_func;
  result_func[idx] = fnctor;
  ++used_result_func;
  return idx;
}

char
apply_value_param(const char *str_value, Param_t *p)
{
  if (strchr(str_value, '.') || strchr(str_value, 'e')) {
    double dval = strtod(str_value, NULL);
    p->d = dval;
    return 'd';
  }

  p->i = strtol(str_value, 0, 0);
  return 'i';
}

int
apply_load_param(const char *str_value, Param_t *p)
{
  if (str_value[0] != '<')
    return -1;

  Global_t *var = global_get(&str_value[1]);
  if (!var) {
    return -1;
  }

  Functor_t fnctor = {
    .call = copy,
    .param[0].p = p,
    .param[1].p = &var->value,
  };
  int idx = add_result_func(fnctor);
  printf("%d: Store into %p variable %s\n", idx, p, str_value);

  return idx;
}

void
execute_init()
{
  memset(result, 0, sizeof(result));
  memset(apply_func, 0, sizeof(apply_func));
  used_apply_func = 0;
  apply_func_condition = ~0;
  memset(result_func, 0, sizeof(result_func));
  used_result_func = 0;
  memset(symbol_load, ~0, sizeof(symbol_load));
}

void
execute_apply_functions()
{
  for (int i = 0; i < used_apply_func; ++i) {
    if (FLAGGED(apply_func_condition, (1 << i)))
      functor_invoke(apply_func[i]);
  }
}

void
execute_load_functions(int symbol_offset)
{
  for (int i = 0; i < PARAM_COUNT; ++i) {
    int func = symbol_load[symbol_offset][i];
    if (func >= 0) {
      functor_invoke(result_func[func]);
    }
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

// apply <?pre> <fn> <p1> <p2> <p3> <...>
void
execute_apply(size_t len, char *input)
{
  const unsigned TOKEN_COUNT = 7;
  char *token[TOKEN_COUNT];
  int tc = tokenize(len, input, TOKEN_COUNT, token);

  if (tc < 2) {
    puts("Usage: apply <?precondition> <func> <param1> <param2> <param3>");
    return;
  }

  int next_token = 1;
  Functor_t *precondition = { 0 };
  if (token[next_token][0] == '?') {
    const Symbol_t *sym = dlfn_get_symbol(&token[next_token][1]);
    if (!sym) {
      puts("precondition not found.");
      return;
    }

    uint32_t sym_offset = (sym - dlfn_symbols);
    static Functor_t fnctor = { .call = fn_filter };
    fnctor.param[0].i = sym_offset;
    precondition = &fnctor;
    ++next_token;
  }

  int matched = 0;
  for (int fi = 0; fi < dlfn_used_symbols; ++fi) {
    const char *filter =
      token[next_token][0] == '*' ? dlfn_symbols[fi].name : token[next_token];

    if (strcmp(filter, dlfn_symbols[fi].name))
      continue;

    ++matched;
    for (int pi = 0; pi < PARAM_COUNT; ++pi) {
      int ti = next_token + pi + 1;
      if (ti >= tc)
        break;
      int func =
        apply_load_param(token[ti], &dlfn_symbols[fi].fnctor.param[pi]);
      symbol_load[fi][pi] = func;
      if (func >= 0) {
        continue;
      }
      apply_value_param(token[ti], &dlfn_symbols[fi].fnctor.param[pi]);
    }
  }
  ++next_token;

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
  Object_t *obj = dlfn_get_object(token[1]);
  if (!obj->address) {
    printf("%s not found.\n", token[1]);
    return;
  }

  if (obj->bytes == 8) {
    long *lp = obj->address;
    double *dp = obj->address;
    printf("%s: %p long %ld double %f\n", token[1], obj->address, *lp, *dp);
  } else if (obj->bytes == 4) {
    signed *p = obj->address;
    printf("%s: %p signed %d\n", token[1], obj->address, *p);
  }
}

void
execute_hash(size_t len, char *input)
{
  memset(hash_result, 0, sizeof(hash_result));
  uint64_t hash_seed = memhash(0, 0);
  for (int i = 0; i < dlfn_used_objects; ++i) {
    hash_seed =
      memhash_cont(hash_seed, dlfn_objects[i].address, dlfn_objects[i].bytes);
    hash_result[i] = hash_seed;
    printf("Hashval %s: %lu\n", dlfn_objects[i].name, hash_seed);
  }
}

void
execute_variable(size_t len, char *input)
{
  const unsigned TOKEN_COUNT = 3;
  char *token[TOKEN_COUNT];
  int tc = tokenize(len, input, TOKEN_COUNT, token);

  if (tc < 3) {
    puts("Usage: variable <name> <value|@func|+|->");
    return;
  }

  printf("%s %s %s\n", token[0], token[1], token[2]);
  Global_t *var = global_get(token[1]);
  if (var) {
    puts("Variable name already in use");
    return;
  }

  Global_t new_var;
  global_init(token[1], &new_var);

  const char *value_str = token[2];
  if (value_str[0] == '@') {
    puts("TODO: function result redirection");
  } else {
    char type = apply_value_param(token[2], &new_var.value);
    new_var.type = type;
    global_append(&new_var);
  }
}

