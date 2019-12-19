#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "functor.c"

#define MAX_VARIABLE_NAME 16

typedef struct {
  Param_t operand[2];
  char operand_type[2];
  union {
    char operator[1];
    uint8_t opcode;
  };
} Op_t;

typedef struct {
  char name[MAX_VARIABLE_NAME];
  Param_t value;
  char type;
  Op_t op;
} Global_t;

// Pure
static size_t
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

static size_t
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

size_t static increment(Global_t *var)
{
  switch (var->type) {
    case 'i':
      return ++var->value.i;
    case 'd':
      return ++var->value.d;
  }

  return 0;
}

size_t static decrement(Global_t *var)
{
  switch (var->type) {
    case 'i':
      return --var->value.i;
    case 'd':
      return --var->value.d;
  }

  return 0;
}

static void
global_init(const char *name, Global_t *var)
{
  *var = (Global_t){0};
  strncpy(var->name, name, MAX_VARIABLE_NAME);
  var->name[MAX_VARIABLE_NAME - 1] = 0;
}

double
perform_op_double(short opcode, double lhv, double rhv)
{
  switch (opcode) {
    case '+':
      return lhv + rhv;
    case '-':
      return lhv - rhv;
    case '>':
      return lhv > rhv;
    case '<':
      return lhv < rhv;
  }

  return 0.0;
}

size_t
perform_op_int(short opcode, size_t lhv, size_t rhv)
{
  switch (opcode) {
    case '+':
      return lhv + rhv;
    case '-':
      return lhv - rhv;
    case '>':
      return lhv > rhv;
    case '<':
      return lhv < rhv;
  }

  return 0;
}

char
perform_op(const Op_t *op, Param_t *out)
{
  char operand_type[2];
  Param_t operand[2];
  for (int i = 0; i < 2; ++i) {
    char op_type = op->operand_type[i];
    switch (op_type) {
      case 'p':
        operand_type[i] = ((const Global_t *)op->operand[i].p)->type;
        operand[i] = ((const Global_t *)op->operand[i].p)->value;
        break;
      default:
        operand_type[i] = op_type;
        operand[i] = op->operand[i];
    }
  }

  char op_type = operand_type[0] & operand_type[1];
  switch (op_type) {
    case 'i':
      out->i = perform_op_int(op->opcode, operand[0].i, operand[1].i);
      break;
    case 'd':
      out->d = perform_op_double(op->opcode, operand[0].d, operand[1].d);
      break;
  }

  return op_type;
}

// File scope
#define MAX_VARIABLE 64
static Global_t global_var[MAX_VARIABLE];
static size_t global_used;

void
global_var_print()
{
  printf("--Variables--\n");
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
  if (global_used >= MAX_VARIABLE) return false;

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
  if (ret) return ret;

  if (global_used >= MAX_VARIABLE) return 0;

  int idx = global_used;
  global_init(name, &global_var[idx]);
  ++global_used;
  return &global_var[idx];
}

void
global_var_ops()
{
  for (int i = 0; i < global_used; ++i) {
    Global_t *var = &global_var[i];
    if (var->op.opcode) {
      Param_t result;
      char result_type = perform_op(&var->op, &result);
      printf("[ result %c ]", result_type);
      switch (result_type) {
        case 'i':
          printf(" %zu -> %zu\n", var->value.i, result.i);
          break;
        case 'd':
          printf(" %f -> %f\n", var->value.d, result.d);
          break;
      }
      var->value = result;
      var->type = result_type;
    }
  }
}
