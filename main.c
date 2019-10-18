#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "float.h"
#include "functor.h"
#include "macro.h"
#include "sys_dlfn.h"
#include "sys_input.h"
#include "sys_loop.c"
#include "sys_notify.h"
#include "sys_record.h"
#include "sys_stats.h"

static const char *dlpath = "code/feature.so";
static const uint32_t simulationMax = UINT32_MAX;
static uint32_t simulationGoal;
static bool exiting;
static Record_t *recording;
static Functor_t apply_func[128];
static size_t used_apply_func;

void
prompt()
{
  sys_dlfnPrintSymbols();
  if (!simulationGoal)
    puts("Simulation is disabled.");
  puts("(q)uit (s)imulation (b)enchmark (a)pply (r)eload>");
}

size_t
increment(size_t *val)
{
  *val += 1;
  return 1;
}

size_t
decrement(size_t *val)
{
  *val -= 1;
  return 1;
}

bool
add_apply_func(Functor_t fnctor)
{
  apply_func[used_apply_func] = fnctor;
  ++used_apply_func;
  return true;
}

void
apply_param(const char *param, Param_t *p)
{
  uint64_t val = strtol(param, 0, 0);
  if (val) {
    printf("param %s val %" PRIu64 "\n", param, val);
    p->i = val;
  } else if (param[0] == '+') {
    printf("param %s increment\n", param);
    Functor_t fnctor = { .call = increment, .param[0].p = &p->i };
    add_apply_func(fnctor);
  } else if (param[0] == '-') {
    printf("param %s decrement\n", param);
    Functor_t fnctor = { .call = decrement, .param[0].p = &p->i };
    add_apply_func(fnctor);
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
print_runtime_perf(size_t length, Stats_t perfStats[length])
{
  for (int i = 0; i < length; ++i) {
    printf("%-20s\t(%5.2e, %5.2e) range\t%5.2e mean ± %4.02f%%\t\n",
           dlfnSymbols[i].name, sys_statsMin(&perfStats[i]),
           sys_statsMax(&perfStats[i]), sys_statsMean(&perfStats[i]),
           100.0 * sys_statsRsDev(&perfStats[i]));
  }
  puts("");
}

void
execute_benchmark()
{
  Stats_t aggregate;
  sys_statsInit(&aggregate);

  const int magnitudes = 10;
  int calls = 1;
  for (int h = 0; h < magnitudes; ++h) {
    Stats_t perfStats[MAX_SYMBOLS];
    for (int i = 0; i < MAX_SYMBOLS; ++i) {
      sys_statsInit(&perfStats[i]);
    }

    for (int i = 0; i < dlfnUsedSymbols; ++i) {
      double sum = 0;
      for (int j = 0; j < calls; ++j) {
        uint64_t startCall = rdtsc();
        functor_invoke(dlfnSymbols[i].fnctor);
        uint64_t endCall = rdtsc();
        double duration = to_double(endCall - startCall);
        sys_statsAddSample(&perfStats[i], duration);
        sum += duration;
      }
      sys_statsAddSample(&aggregate, sum);
    }

    printf("--per 10e%d\n", h);
    print_runtime_perf(dlfnUsedSymbols, perfStats);

    if (aggregate.max > 10000000.0)
      break;

    calls *= 10;
  }

  puts("benchmark threshold reached.");
}

void
inputEvent(size_t len, char *input)
{
  // These events are not recorded
  switch (input[0]) {
  case 'p':
    sys_recordPlayback(recording, inputEvent);
    return;
  case 'r':
    simulationGoal = 0;
    sys_loopHalt();
    return;
  case 'b':
    execute_benchmark();
    return;
  }

  sys_recordAppend(recording, len, input);

  switch (input[0]) {
  case 'q':
    simulationGoal = 0;
    exiting = true;
    sys_loopHalt();
    return;
  case 's':
    simulationGoal = simulationMax;
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

  simulationGoal = 0;
  sys_loopHalt();
}

void
execute_simulation()
{
  char *watchDirs[] = { "code" };
  double perf[MAX_SYMBOLS];
  Stats_t perfStats[MAX_SYMBOLS];
  for (int i = 0; i < MAX_SYMBOLS; ++i) {
    sys_statsInit(&perfStats[i]);
  }
  memset(apply_func, 0, sizeof(apply_func));
  used_apply_func = 0;

  sys_loopInit(10);
  sys_loopPrintStatus();
  sys_notifyInit(IN_CLOSE_WRITE, ARRAY_LENGTH(watchDirs), watchDirs);
  sys_dlfnInit(dlpath);
  sys_dlfnOpen();
  simulationGoal = simulationMax;
  sys_recordSeekW(recording, 0u);
  sys_recordPlaybackAll(recording, inputEvent);
  sys_inputInit();
  prompt();
  while (sys_loopRun()) {
    sys_inputPoll(inputEvent);
    sys_notifyPoll(notifyEvent);

    if (simulationGoal <= sys_loopFrame()) {
      sys_loopPause();
      continue;
    }

    for (int i = 0; i < used_apply_func; ++i) {
      functor_invoke(apply_func[i]);
    }

    for (int i = 0; i < dlfnUsedSymbols; ++i) {
      uint64_t startCall = rdtsc();
      functor_invoke(dlfnSymbols[i].fnctor);
      uint64_t endCall = rdtsc();
      perf[i] = to_double(endCall - startCall);
    }

    for (int i = 0; i < dlfnUsedSymbols; ++i) {
      sys_statsAddSample(&perfStats[i], perf[i]);
    }

    sys_loopSync();
  }
  puts("--simulation performance");
  print_runtime_perf(dlfnUsedSymbols, perfStats);
  sys_loopPrintStatus();
  sys_loopShutdown();
  sys_notifyShutdown();
  sys_dlfnShutdown();
  sys_inputShutdown();
}

int
main(int argc, char **argv)
{
  recording = sys_recordAlloc();
  while (!exiting) {
    execute_simulation();
  }
  sys_recordFree(recording);

  return 0;
}
