#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "sys_dlfn.h"

static char *
skipWhitespace(char *str)
{
  while (*str == ' ' || *str == '\t')
    ++str;

  return str;
}

static char *
seekLF(char *str)
{
  while (*str != '\n' && *str != 0)
    ++str;
  return str;
}

static inline void
trySymbol()
{
  char buffer[128];
  if (fgets(buffer, sizeof(buffer), stdin)) {
    char *str = skipWhitespace(buffer);
    char *end = seekLF(str);
    *end = 0;
    sys_dlfnCall(str);
  }
}

void
handler(char c, bool *stop)
{
  switch (c) {
  case 'q':
    *stop = true;
    return;
  case 'r':
    sys_dlfnClose();
    sys_dlfnOpen();
    return;
  case 'c':
    trySymbol();
    return;
  }
}

void
prompt()
{
  sys_dlfnPrintSymbols();
  // dl_iterate_phdr(phdr_callback, NULL);
  puts("(q)uit (r)eload (c)all >");
}

void
waitForInput()
{
  bool stop = false;
  for (char c = 0;; c = getc(stdin)) {
    if (c == '\n')
      continue;
    handler(c, &stop);
    if (stop)
      return;
    prompt();
  }
}

int
main(int argc, char **argv)
{
  sys_dlfnClose();

  waitForInput();

  return 0;
}
