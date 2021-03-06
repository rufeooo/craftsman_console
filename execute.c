#pragma once

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "dlfn.c"
#include "float.c"
#include "functor.c"
#include "global.c"
#include "hash.c"
#include "macro.h"
#include "rdtsc.h"
#include "stats.c"

// extern
extern long int strtol(const char *__restrict __nptr,
                       char **__restrict __endptr, int __base) __THROW
    __nonnull((1));
extern double strtod(const char *__restrict __nptr,
                     char **__restrict __endptr) __THROW __nonnull((1));

static uint64_t hash_objects[MAX_SYMBOLS];
static size_t result[MAX_SYMBOLS];
static int load_param_handle[MAX_SYMBOLS][PARAM_COUNT];
static int save_result_handle[MAX_SYMBOLS];

#define MAX_FUNC (MAX_SYMBOLS * PARAM_COUNT)
static Functor_t result_func[MAX_FUNC];
static size_t used_result_func;

size_t
noop()
{
  return 0;
}

size_t
copy(Param_t *dst, const Param_t *src)
{
  printf("Copy %p <- %p\n", dst, src);
  *dst = *src;
  printf("Copied %lu <- %lu\n", dst->i, src->i);
  return 1;
}

int
tokenize(size_t len, char *input, size_t token_count, char *token[token_count])
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
add_result_func(Functor_t fnctor)
{
  if (used_result_func >= MAX_FUNC) return -1;

  int idx = used_result_func;
  result_func[idx] = fnctor;
  ++used_result_func;
  return idx;
}

char
set_value_param(const char *str_value, Param_t *p)
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
set_load_param(const char *str_value, Param_t *p)
{
  Global_t *var = global_get(str_value);
  if (!var) {
    return 0;
  }

  var->type = 'i';
  Functor_t fnctor = {
      .call = copy,
      .param[0].p = p,
      .param[1].p = &var->value,
  };
  int idx = add_result_func(fnctor);
  printf("%d: Store into %p variable %s\n", idx, p, str_value);

  return idx;
}

int
set_store_param(const char *str_value, size_t *result_ptr)
{
  Global_t *var = global_get_or_create(str_value);
  if (!var) {
    return 0;
  }

  var->type = 'i';
  Functor_t fnctor = {
      .call = copy,
      .param[0].p = &var->value,
      .param[1].p = result_ptr,
  };
  int idx = add_result_func(fnctor);
  printf("%d: Store into variable %s ptr %p\n", idx, str_value, result_ptr);

  return idx;
}

void
execute_init()
{
  memset(result, 0, sizeof(result));
  memset(result_func, 0, sizeof(result_func));
  used_result_func = 0;
  memset(load_param_handle, 0, sizeof(load_param_handle));

  Functor_t np = {.call = noop};
  add_result_func(np);
}

void
call_load_param(int symbol_offset)
{
  for (int i = 0; i < PARAM_COUNT; ++i) {
    int func = load_param_handle[symbol_offset][i];
    functor_invoke(result_func[func]);
  }
}

void
call_store_result(int symbol_offset)
{
  int func = save_result_handle[symbol_offset];
  functor_invoke(result_func[func]);
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

    for (int i = 0; i < dlfn_used_function; ++i) {
      double sum = 0;
      for (int j = 0; j < calls; ++j) {
        uint64_t startCall = rdtsc();
        functor_invoke(dlfn_function[i].fnctor);
        uint64_t endCall = rdtsc();
        double duration = to_double(endCall - startCall);
        stats_add(duration, &perfStats[i]);
        sum += duration;
      }
      stats_add(sum, &aggregate);
    }

    printf("--per 10e%d\n", h);
    for (int i = 0; i < dlfn_used_function; ++i) {
      printf("%-20s\t(%5.2e, %5.2e) range\t%5.2e mean ± %4.02f%%\t\n",
             dlfn_function[i].name, stats_min(&perfStats[i]),
             stats_max(&perfStats[i]), stats_mean(&perfStats[i]),
             100.0 * stats_unbiased_rs_dev(&perfStats[i]));
    }
    puts("");

    if (aggregate.max > 10000000.0) break;

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
  int token_count = tokenize(len, input, TOKEN_COUNT, token);

  if (token_count == 1) return SIMULATION_MAX;

  uint64_t val = strtol(token[1], 0, 0);
  return val;
}

// parameter <func> <p1> <p2> <p3> <...>
void
execute_parameter(size_t len, char *input)
{
  const unsigned TOKEN_COUNT = 6;
  char *token[TOKEN_COUNT];
  int token_count = tokenize(len, input, TOKEN_COUNT, token);

  if (token_count < 2) {
    puts("Usage: parameter <func> <param1> <param2> <param3>");
    return;
  }

  int matched = 0;
  for (int fi = 0; fi < dlfn_used_function; ++fi) {
    const char *filter = token[1][0] == '*' ? dlfn_function[fi].name : token[1];

    if (strcmp(filter, dlfn_function[fi].name)) continue;

    ++matched;
    for (int pi = 0; pi < PARAM_COUNT; ++pi) {
      int token_index = pi + 2;
      if (token_index >= token_count) break;

      int func = set_load_param(token[token_index],
                                &dlfn_function[fi].fnctor.param[pi]);
      load_param_handle[fi][pi] = func;
      if (func > 0) continue;

      set_value_param(token[token_index], &dlfn_function[fi].fnctor.param[pi]);
    }
  }

  if (!matched) {
    printf("Function not found (%s).\n", token[1]);
    return;
  }
}

void
execute_object(size_t len, char *input)
{
  const unsigned TOKEN_COUNT = 2;
  char *token[TOKEN_COUNT];
  int token_count = tokenize(len, input, TOKEN_COUNT, token);

  if (token_count < 2) {
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
  memset(hash_objects, 0, sizeof(hash_objects));
  uint64_t hash_val = memhash(0, 0);
  for (int i = 0; i < dlfn_used_object; ++i) {
    hash_val =
        memhash_cont(hash_val, dlfn_object[i].address, dlfn_object[i].bytes);
    hash_objects[i] = hash_val;
    printf("Hashval %s: %lu\n", dlfn_object[i].name, hash_val);
  }
}

void
execute_variable(size_t len, char *input)
{
  const unsigned TOKEN_COUNT = 3;
  char *token[TOKEN_COUNT];
  int token_count = tokenize(len, input, TOKEN_COUNT, token);

  if (token_count < 3) {
    puts("Usage: variable <name> <value>");
    return;
  }

  Global_t *var = global_get_or_create(token[1]);
  if (!var) {
    puts("Variable is null - out of reserved space.");
    return;
  }

  char type = set_value_param(token[2], &var->value);
  var->type = type;
}

void
execute_result(size_t len, char *input)
{
  const unsigned TOKEN_COUNT = 3;
  char *token[TOKEN_COUNT];
  int token_count = tokenize(len, input, TOKEN_COUNT, token);

  if (token_count < 3) {
    puts("Usage: result <func> <variable>");
    return;
  }

  printf("%s %s %s\n", token[0], token[1], token[2]);
  Function_t *sym = dlfn_get_symbol(token[1]);
  if (!sym) {
    printf("Cannot store result, function %s not found.\n", token[1]);
    return;
  }

  int sym_offset = sym - dlfn_function;
  int func = set_store_param(token[2], &result[sym_offset]);
  save_result_handle[sym_offset] = func;
}

void
execute_mutation(size_t len, char *input)
{
  const unsigned TOKEN_COUNT = 4;
  char *token[TOKEN_COUNT];
  int token_count = tokenize(len, input, TOKEN_COUNT, token);

  if (token_count < TOKEN_COUNT) {
    puts("Usage: mutation <variable> [<,>,+,-] <variable|constant>");
    return;
  }

  Global_t *lhv = global_get(token[1]);
  const Global_t *rhv = global_get(token[3]);

  if (!lhv) {
    printf("Variable not found %s\n", token[1]);
    return;
  }

  Op_t op = {.operand_type[0] = 'p',
             .operand[0].cp = lhv,
             .operator[0] = token[2][0] };

  if (rhv) {
    op.operand_type[1] = 'p';
    op.operand[1].cp = rhv;
  } else {
    op.operand_type[1] = set_value_param(token[3], &op.operand[1]);
  }

  lhv->op = op;
}

void
execute_condition(size_t len, char *input)
{
  const unsigned TOKEN_COUNT = 4;
  char *token[TOKEN_COUNT];
  int token_count = tokenize(len, input, TOKEN_COUNT, token);

  if (token_count < TOKEN_COUNT) {
    puts("Usage: condition <variable|constant> [<,>,+,-] <variable|constant>");
    return;
  }

  const Global_t *lhv = global_get(token[1]);
  const Global_t *rhv = global_get(token[3]);

  Op_t op;
  if (lhv) {
    op.operand_type[0] = 'p';
    op.operand[0].cp = lhv;
  } else {
    op.operand_type[0] = set_value_param(token[1], &op.operand[0]);
  }
  if (rhv) {
    op.operand_type[1] = 'p';
    op.operand[1].cp = rhv;
  } else {
    op.operand_type[1] = set_value_param(token[3], &op.operand[1]);
  }
  op.operator[0] = token[2][0];

  Param_t result;
  char result_type = perform_op(&op, &result);
  printf("[ result_type %c ]: %f %zu\n", result_type, result.d, result.i);
}
