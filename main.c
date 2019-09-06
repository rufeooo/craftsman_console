#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define __USE_GNU
#include <dlfcn.h>
#include <link.h>

#include "sys_dlfn.c"

void
handler(char c, bool *stop)
{
  switch (c) {
  case 'q':
    *stop = true;
    return;
  case 'r':
    closeLibrary();
    openLibrary();
    return;
  case 'c':
    trySymbol();
    return;
  }
}

void
prompt()
{
  if (dlp)
    printSymbols();
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
  closeLibrary();

  waitForInput();

  return 0;
}
