#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sys_dlfn.h"
#include "sys_input.h"

static bool running = true;

static char *
skipWhitespace(char *str)
{
  while (*str == ' ' || *str == '\t')
    ++str;

  return str;
}

static inline void
trySymbol(char *input)
{
  char *firstSpace = strchr(input, ' ');
  if (!firstSpace)
    return;
  char *str = skipWhitespace(firstSpace);
  printf("Trycall %s\n", str);
  sys_dlfnCall(str);
}

void
prompt()
{
  sys_dlfnPrintSymbols();
  // dl_iterate_phdr(phdr_callback, NULL);
  puts("(q)uit (r)eload (c)all >");
}

void
inputEvent(size_t len, char *input)
{
  switch (input[0]) {
  case 'q':
    running = false;
    return;
  case 'r':
    sys_dlfnClose();
    sys_dlfnOpen();
    return;
  case 'c':
    trySymbol(input);
    return;
  }

  prompt();
}

int
main(int argc, char **argv)
{
  sys_dlfnClose();

  sys_inputInit();
  prompt();
  while (running) {
    sys_inputPoll(inputEvent);
  }
  sys_inputShutdown();

  sys_dlfnClose();

  return 0;
}
