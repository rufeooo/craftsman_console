#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "macro.h"
#include "sys_dlfn.h"
#include "sys_input.h"
#include "sys_notify.h"

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
  puts("(q)uit (c)all >");
}

void
inputEvent(size_t len, char *input)
{
  switch (input[0]) {
  case 'q':
    running = false;
    return;
  case 'c':
    trySymbol(input);
    return;
  }

  prompt();
}

void
notifyEvent(int idx, const struct inotify_event *event)
{
  printf("File change %s\n", event->name);
  if (!strstr(event->name, "feature.so"))
    return;

  sys_dlfnClose();
  sys_dlfnOpen();
}

int
main(int argc, char **argv)
{
  char *watchDirs[] = { "code" };
  sys_dlfnClose();

  sys_notifyInit(IN_CLOSE_WRITE, LENGTHOF(watchDirs), watchDirs);
  sys_inputInit();
  prompt();
  while (running) {
    sys_inputPoll(inputEvent);
    sys_notifyPoll(notifyEvent);
  }
  sys_inputShutdown();

  sys_dlfnClose();

  return 0;
}
