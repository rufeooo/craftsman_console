#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dlfn.h"
#include "float.h"
#include "functor.h"
#include "input.h"
#include "macro.h"
#include "notify.h"
#include "record.h"
#include "stats.h"
#include "loop.c"

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
  dlfn_print_symbols();
  printf("Simulation will run until frame %d.\n", simulationGoal);
  puts("(q)uit (s)imulation (b)enchmark (a)pply (r)eload>");
}

size_t
increment(size_t *val)
{
  *val += 1;
  return 1;
}

size_t
left_shift(size_t *val)
{
  *val <<= 1;
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
  } else if (param[0] == '<') {
    printf("param %s left_shift\n", param);
    Functor_t fnctor = { .call = left_shift, .param[0].p = &p->i };
    add_apply_func(fnctor);
  } else { // TODO
    printf("string param unhandled %s\n", param);
  }
}

int
tokenize(size_t len, char *input, size_t token_count,
         char *token[token_count])
{
  char *iter = input;
  char *end = input + len;
  token[0] = input;
  int tc = 1;

  for (; iter < end && tc < token_count; ++iter) {
    if (*iter == ' ') {
      *iter = 0;
      token[tc++] = iter + 1;
    }
  }

  return tc;
}

// simulation <until frame>
void
execute_simulation(size_t len, char *input)
{
  const unsigned TOKEN_COUNT = 2;
  char *token[TOKEN_COUNT] = { [1] = input };
  int tc = tokenize(len, input, TOKEN_COUNT, token);

  simulationGoal = simulationMax;

  if (tc == 1)
    return;

  uint64_t val = strtol(token[1], 0, 0);
  simulationGoal = val;
}

// apply <fn> <p1> <p2> <p3> <...>
void
execute_apply(size_t len, char *input)
{
  const unsigned TOKEN_COUNT = 6;
  char *token[TOKEN_COUNT];
  int tc = tokenize(len, input, TOKEN_COUNT, token);

  if (tc < 2) {
    puts("Usage: apply <func> <param1> <param2> <param3>");
    return;
  }

  Functor_t *f = dlfn_get_symbol(token[1]);
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
           dlfnSymbols[i].name, stats_min(&perfStats[i]),
           stats_max(&perfStats[i]), stats_mean(&perfStats[i]),
           100.0 * stats_rs_dev(&perfStats[i]));
  }
  puts("");
}

void
execute_benchmark()
{
  Stats_t aggregate;
  stats_init(&aggregate);

  const int magnitudes = 10;
  int calls = 1;
  for (int h = 0; h < magnitudes; ++h) {
    Stats_t perfStats[MAX_SYMBOLS];
    for (int i = 0; i < MAX_SYMBOLS; ++i) {
      stats_init(&perfStats[i]);
    }

    for (int i = 0; i < dlfnUsedSymbols; ++i) {
      double sum = 0;
      for (int j = 0; j < calls; ++j) {
        uint64_t startCall = rdtsc();
        functor_invoke(dlfnSymbols[i].fnctor);
        uint64_t endCall = rdtsc();
        double duration = to_double(endCall - startCall);
        stats_sample_add(&perfStats[i], duration);
        sum += duration;
      }
      stats_sample_add(&aggregate, sum);
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
input_callback(size_t len, char *input)
{
  // These events are not recorded
  switch (input[0]) {
  case 'p':
    record_playback(recording, input_callback);
    return;
  case 'r':
    simulationGoal = 0;
    loop_halt();
    return;
  case 'b':
    execute_benchmark();
    return;
  }

  record_append(recording, len, input);

  switch (input[0]) {
  case 'q':
    simulationGoal = 0;
    exiting = true;
    loop_halt();
    return;
  case 's':
    execute_simulation(len, input);
    return;
  case 'a':
    execute_apply(len, input);
    return;
  }

  prompt();
}

void
notify_callback(int idx, const struct inotify_event *event)
{
  printf("File change %s\n", event->name);
  if (!strstr(dlpath, event->name))
    return;

  simulationGoal = 0;
  loop_halt();
}

void
game_simulation()
{
  char *watchDirs[] = { "code" };
  double perf[MAX_SYMBOLS];
  Stats_t perfStats[MAX_SYMBOLS];
  for (int i = 0; i < MAX_SYMBOLS; ++i) {
    stats_init(&perfStats[i]);
  }
  memset(apply_func, 0, sizeof(apply_func));
  used_apply_func = 0;

  loop_init(10);
  loop_print_status();
  notify_init(IN_CLOSE_WRITE, ARRAY_LENGTH(watchDirs), watchDirs);
  dlfn_init(dlpath);
  dlfn_open();
  simulationGoal = 0;
  record_seek_write(recording, 0u);
  record_playback_all(recording, input_callback);
  input_init();
  prompt();
  while (loop_run()) {
    input_poll(input_callback);
    notify_poll(notify_callback);

    if (simulationGoal <= loop_frame()) {
      loop_pause();
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
      stats_sample_add(&perfStats[i], perf[i]);
    }

    loop_sync();
  }
  puts("--simulation performance");
  print_runtime_perf(dlfnUsedSymbols, perfStats);
  loop_print_status();
  loop_shutdown();
  notify_shutdown();
  dlfn_shutdown();
  input_shutdown();
}

int
main(int argc, char **argv)
{
  recording = record_alloc();
  while (!exiting) {
    game_simulation();
  }
  record_free(recording);

  return 0;
}
