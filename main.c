#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "macro.h"
#include "sys_dlfn.h"
#include "sys_input.h"
#include "sys_loop.c"
#include "sys_notify.h"

static const char *dlpath = "code/feature.so";

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
    sys_loopHalt();
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
  if (!strstr(dlpath, event->name))
    return;

  sys_dlfnClose();
  sys_dlfnOpen();
}

int
main(int argc, char **argv)
{
  char *watchDirs[] = { "code" };

  sys_loopInit(10);
  sys_loopPrintStatus();
  sys_notifyInit(IN_CLOSE_WRITE, LENGTHOF(watchDirs), watchDirs);
  sys_dlfnInit(dlpath);
  sys_dlfnOpen();
  sys_inputInit();
  prompt();
  while (sys_loopRun()) {
    sys_inputPoll(inputEvent);
    sys_notifyPoll(notifyEvent);
    sys_loopSync();
  }
  sys_notifyShutdown();
  sys_dlfnShutdown();
  sys_inputShutdown();
  sys_loopShutdown();
  sys_loopPrintStatus();

  return 0;
}
