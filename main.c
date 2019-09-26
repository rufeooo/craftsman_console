#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "functor.h"
#include "macro.h"
#include "sys_dlfn.h"
#include "sys_input.h"
#include "sys_loop.c"
#include "sys_notify.h"
#include "sys_record.h"

static const char *dlpath = "code/feature.so";
static const bool simulationDefault = false;
static bool simulation = simulationDefault;

void
prompt()
{
  sys_dlfnPrintSymbols();
  if (!simulation)
    puts("Simulation is disabled.");
  puts("(q)uit (s)imulation (a)pply>");
}

void
apply_param(const char *param, Param_t *p)
{
  uint64_t val = strtol(param, 0, 0);
  if (val) {
    printf("param %s val %lu\n", param, val);
    p->i = val;
  } else { // TODO
    printf("string param unhandled %s\n", param);
  }
}

// apply <fn> <p1> <p2> <p3> <...>
void
apply(size_t len, char *input)
{
  const unsigned TOKEN_COUNT = 6;
  char *iter = input;
  char *end = input + len;
  char *token[TOKEN_COUNT] = { input };
  int tc = 1;

  for (; iter < end && tc < TOKEN_COUNT; ++iter) {
    if (*iter == ' ') {
      *iter = 0;
      token[tc++] = iter + 1;
    }
  }

  if (tc < 2) {
    puts("Usage: apply <func> <param1> <param2> <param3>");
    return;
  }

  Functor_t *f = sys_dlfnGetSymbol(token[1]);
  if (!f) {
    printf("Failure to apply: function not found (%s).\n", token[1]);
    return;
  }
  for (int pi = 0; pi < 3; ++pi) {
    int ti = 2 + pi;
    if (ti >= tc)
      break;
    apply_param(token[ti], &f->param[pi]);
  }
}

void
inputEvent(size_t len, char *input)
{
  // Never record playback requests
  if (input[0] == 'p') {
    sys_recordPlayback(inputEvent);
    return;
  }

  sys_recordAppend(len, input);

  switch (input[0]) {
  case 'q':
    simulation = false;
    sys_loopHalt();
    return;
  case 's':
    simulation = !simulation;
    return;
  case 'a':
    apply(len, input);
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
  simulation = simulationDefault;
  sys_recordPlayback(inputEvent);

  prompt();
}

int
main(int argc, char **argv)
{
  char *watchDirs[] = { "code" };
  uint64_t perf[MAX_SYMBOLS];

  sys_loopInit(10);
  sys_loopPrintStatus();
  sys_notifyInit(IN_CLOSE_WRITE, ARRAY_LENGTH(watchDirs), watchDirs);
  sys_dlfnInit(dlpath);
  sys_dlfnOpen();
  sys_recordInit();
  sys_inputInit();
  prompt();
  while (sys_loopRun()) {
    sys_inputPoll(inputEvent);
    sys_notifyPoll(notifyEvent);

    if (!simulation) {
      sys_loopPause();
      continue;
    }

    for (int i = 0; i < dlfnUsedSymbols; ++i) {
      uint64_t startCall = rdtsc();
      functor_invoke(dlfnSymbols[i].fnctor);
      uint64_t endCall = rdtsc();
      perf[i] = endCall - startCall;
    }
    for (int i = 0; i < dlfnUsedSymbols; ++i) {
      printf("%s: %lu ", dlfnSymbols[i].name, perf[i]);
    }
    puts("");
    sys_loopSync();
  }
  sys_notifyShutdown();
  sys_dlfnShutdown();
  sys_inputShutdown();
  sys_recordShutdown();
  sys_loopShutdown();
  sys_loopPrintStatus();

  return 0;
}
