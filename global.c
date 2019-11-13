#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "functor.h"

#define MAX_VARIABLE_NAME 16

typedef struct {
  char name[MAX_VARIABLE_NAME];
  Param_t value;
  char type;
  Functor_t condition;
  Functor_t mutation;
} Global_t;

// Pure
size_t
less_than(const Global_t *lhv, const Global_t *rhv)
{
  switch (lhv->type & rhv->type) {
  case 'i':
    return lhv->value.i < rhv->value.i;
  case 'd':
    return lhv->value.d < rhv->value.d;
  }

  return 0;
}

size_t
greater_than(const Global_t *lhv, const Global_t *rhv)
{
  switch (lhv->type & rhv->type) {
  case 'i':
    return lhv->value.i > rhv->value.i;
  case 'd':
    return lhv->value.d > rhv->value.d;
  }

  return 0;
}

size_t
increment(Global_t *var)
{
  switch (var->type) {
  case 'i':
    return ++var->value.i;
  case 'd':
    return ++var->value.d;
  }

  return 0;
}

size_t
decrement(Global_t *var)
{
  switch (var->type) {
  case 'i':
    return --var->value.i;
  case 'd':
    return --var->value.d;
  }

  return 0;
}

// File scope
#define MAX_VARIABLE 64
static Global_t global_var[MAX_VARIABLE];
static size_t global_used;

static void
global_init(const char *name, Global_t *var)
{
  *var = (Global_t){ 0 };
  strncpy(var->name, name, MAX_VARIABLE_NAME);
  var->name[MAX_VARIABLE_NAME - 1] = 0;
}

void
global_var_print()
{
  for (int i = 0; i < global_used; ++i) {
    printf("[ var '%c' ] %s ( ", global_var[i].type, global_var[i].name);
    switch (global_var[i].type) {
    case 'i':
      printf("%lu", global_var[i].value.i);
      break;
    case 'd':
      printf("%f", global_var[i].value.d);
      break;
    }
    printf(" )\n");
  }
}

bool
global_append(Global_t *var)
{
  if (global_used >= MAX_VARIABLE)
    return false;

  global_var[global_used] = *var;
  ++global_used;
  global_var_print();
  return true;
}

Global_t *
global_get(const char *name)
{
  for (int i = 0; i < global_used; ++i) {
    if (strncmp(name, global_var[i].name, MAX_VARIABLE_NAME) == 0) {
      return &global_var[i];
    }
  }

  return 0;
}

Global_t *
global_get_or_create(const char *name)
{
  Global_t *ret = global_get(name);
  if (ret)
    return ret;

  if (global_used >= MAX_VARIABLE)
    return 0;

  int idx = global_used;
  global_init(name, &global_var[idx]);
  ++global_used;
  return &global_var[idx];
}

Global_t *
global_mutator(const char *name, char op)
{
  Global_t *var = global_get(name);
  if (!var)
    return 0;

  if (op == '+') {
    puts("increment mutation");
    var->mutation = (Functor_t){ .call = increment, .param[0].p = var };
  } else if (op == '-') {
    puts("decrement mutation");
    var->mutation = (Functor_t){ .call = decrement, .param[0].p = var };
  } else {
    puts("no mutation");
    var->mutation = (Functor_t){ 0 };
  }

  return var;
}

void
global_condition(char op, Global_t *var)
{
  if (op == '<') {
    puts("less_than condition");
    var->condition = (Functor_t){ .call = less_than };
  } else if (op == '>') {
    puts("greater_than condition");
    var->condition = (Functor_t){ .call = greater_than };
  } else {
    puts("no condition");
    var->condition = (Functor_t){ 0 };
  }
}

void
global_call_mutators()
{
  for (int i = 0; i < global_used; ++i) {
    if (global_var[i].condition.call
        && functor_invoke(global_var[i].condition) == 0)
      continue;
    if (global_var[i].mutation.call)
      functor_invoke(global_var[i].mutation);
  }
}

