#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "functor.h"

#define MAX_VARIABLE_NAME 16

typedef struct {
  char name[MAX_VARIABLE_NAME];
  Param_t value;
  char type;
} Global_t;

#define MAX_VARIABLE 64
Global_t global_var[MAX_VARIABLE];
size_t global_used;

static void
global_init(const char *name, Global_t *var)
{
  *var = (Global_t){ 0 };
  strncpy(var->name, name, MAX_VARIABLE_NAME);
  var->name[MAX_VARIABLE_NAME - 1] = 0;
}

void
global_print()
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
  global_print();
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

