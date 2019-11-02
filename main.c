#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "connection.c"
#include "dlfn.h"
#include "float.h"
#include "functor.h"
#include "hash.h"
#include "input.h"
#include "loop.c"
#include "macro.h"
#include "notify.h"
#include "record.h"
#include "stats.h"

#define MAX_FUNC 128
#define GAME_BUFFER 4096

static bool buffering = false;
static uint32_t writes;
static const char *dlpath = "code/feature.so";
static const uint32_t simulationMax = UINT32_MAX;
static uint32_t simulationGoal;
static bool exiting;
static RecordRW_t *input_rw;
static Functor_t apply_func[MAX_FUNC];
static size_t used_apply_func;
static Functor_t result_func[MAX_FUNC];
static size_t used_result_func;

typedef struct {
  uint32_t turn_nearest;
  uint32_t turn_farthest;
  uint32_t turn_command_count;
  char *command[MAX_PLAYER];
  size_t command_len[MAX_PLAYER];
} CommandFrame_t;

#define FRAME_BUF_LEN 4096
bool
build_frame(size_t player_count, RecordRW_t recording[player_count],
            CommandFrame_t *out_state)
{
  static char buffer[FRAME_BUF_LEN];
  CommandFrame_t ret_state = { .turn_nearest = loop_input_queue_max(),
                               .turn_farthest = 0,
                               .turn_command_count = player_count };
  char *buffer_write = buffer;
  char *buffer_end = buffer + FRAME_BUF_LEN;
  for (int i = 0; i < player_count; ++i) {
    uint32_t rcmd = recording[i].read.command_count;
    uint32_t wcmd = recording[i].write.command_count;

    ret_state.turn_farthest = MAX(wcmd - rcmd, ret_state.turn_farthest);
    ret_state.turn_nearest = MIN(wcmd - rcmd, ret_state.turn_nearest);
  }

  for (int i = 0; i < player_count; ++i) {
    size_t cmd_len = 0;
    const char *cmd =
      record_read(recording[i].rec, &recording[i].read, &cmd_len);
    if (buffer_write + cmd_len >= buffer_end)
      return false;
    memcpy(buffer_write, cmd, cmd_len);
    buffer_write[cmd_len] = 0;

    ret_state.command[i] = buffer_write;
    ret_state.command_len[i] = cmd_len;
    buffer_write += cmd_len + 1;
  }

  *out_state = ret_state;

  return true;
}

uint32_t
game_players(RecordRW_t recording[static MAX_PLAYER])
{
  uint32_t count = 0;
  for (int i = 0; i < MAX_PLAYER; ++i) {
    count += (recording[i].rec != 0);
  }

  return count;
}

size_t
print_result(const Symbol_t *sym, size_t r)
{
  printf("0x%zX %s 0x%zX 0x%zX 0x%zX\n", r, sym->name, sym->fnctor.param[0].i,
         sym->fnctor.param[1].i, sym->fnctor.param[2].i);
  return 0;
}

void
prompt(int player_count)
{
  dlfn_print_symbols();
  printf("Simulation will run until frame %d.\n", simulationGoal);
  printf("Player Count: %d\n", player_count);
  puts(
    "(q)uit (i)nfo (s)imulation (b)enchmark (a)pply (p)rint (h)ash (o)bject "
    "(r)eload>");
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
  if (used_apply_func >= MAX_FUNC)
    return false;

  apply_func[used_apply_func] = fnctor;
  ++used_apply_func;
  return true;
}

bool
add_result_func(Functor_t fnctor)
{
  if (used_result_func >= MAX_FUNC)
    return false;

  result_func[used_result_func] = fnctor;
  ++used_result_func;
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

  int matched = 0;
  for (int fi = 0; fi < dlfnUsedSymbols; ++fi) {
    const char *filter = token[1][0] == '*' ? dlfnSymbols[fi].name : token[1];

    if (strcmp(filter, dlfnSymbols[fi].name))
      continue;

    ++matched;
    for (int pi = 0; pi < PARAM_COUNT; ++pi) {
      int ti = 2 + pi;
      if (ti >= tc)
        break;
      apply_param(token[ti], &dlfnSymbols[fi].fnctor.param[pi]);
    }
  }

  if (!matched) {
    printf("Failure to apply: function not found (%s).\n", token[1]);
    return;
  }
}

void
execute_print(size_t len, char *input)
{
  Functor_t fnctor = { .call = print_result };
  add_result_func(fnctor);

  puts("function results will be printed.");
}

void
execute_object(size_t len, char *input)
{
  const unsigned TOKEN_COUNT = 2;
  char *token[TOKEN_COUNT];
  int tc = tokenize(len, input, TOKEN_COUNT, token);

  if (tc < 2) {
    puts("Usage: object <name>");
    return;
  }
  void *addr = dlfn_get_object(token[1]);
  if (!addr) {
    printf("%s not found.\n", token[1]);
    return;
  }
  long *vp = addr;
  printf("%s: %p value %lu\n", token[1], addr, *vp);
}

void
execute_hash(size_t len, char *input)
{
  uint64_t hash_seed = 5381;
  for (int i = 0; i < dlfnUsedObjects; ++i) {
    hash_seed =
      memhash_cont(hash_seed, dlfnObjects[i].address, dlfnObjects[i].bytes);
  }
  printf("Hashval %lu\n", hash_seed);
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
  case 'r':
    simulationGoal = 0;
    loop_halt();
    return;
  case 'q':
    simulationGoal = 0;
    exiting = true;
    loop_halt();
    return;
  case 'i':
    loop_print_status();
    return;
  }

  record_append(input_rw->rec, len, input, &input_rw->write);
}

void
game_action(size_t len, char *input)
{
  switch (input[0]) {
  case 'b':
    execute_benchmark();
    return;
  case 's':
    execute_simulation(len, input);
    return;
  case 'a':
    execute_apply(len, input);
    return;
  case 'p':
    execute_print(len, input);
    return;
  case 'o':
    execute_object(len, input);
    return;
  case 'h':
    execute_hash(len, input);
    return;
  }
}

void
input_to_network(size_t len, char *input)
{
  if (input[len] != 0)
    CRASH();

  ++len;
  ++writes;

  ssize_t written = network_write(len, input);
  buffering = buffering | (written != len);
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
game_simulation(RecordRW_t game_record[static MAX_PLAYER])
{
  const int player_count = game_players(game_record);
  char *watchDirs[] = { "code" };
  size_t result[MAX_SYMBOLS];
  double perf[MAX_SYMBOLS];
  Stats_t perfStats[MAX_SYMBOLS];
  Stats_t net_perf;
  for (int i = 0; i < MAX_SYMBOLS; ++i) {
    stats_init(&perfStats[i]);
  }
  stats_init(&net_perf);
  memset(apply_func, 0, sizeof(apply_func));
  used_apply_func = 0;
  memset(result_func, 0, sizeof(result_func));
  used_result_func = 0;
  connection_reset_read();

  loop_init(10);
  loop_print_status();
  notify_init(IN_CLOSE_WRITE, ARRAY_LENGTH(watchDirs), watchDirs);
  dlfn_init(dlpath);
  dlfn_open();
  simulationGoal = 0;
  input_init();
  prompt(player_count);
  while (loop_run()) {
    loop_print_frame();

    notify_poll(notify_callback);
    input_poll(input_callback);

    if (!record_peek(input_rw->rec, &input_rw->read))
      record_append(input_rw->rec, 0, 0, &input_rw->write);

    CommandFrame_t frame;
    if (!build_frame(player_count, game_record, &frame))
      CRASH();
    printf("[ %d farthest ] [ %d nearest ] \n", frame.turn_farthest,
           frame.turn_nearest);
    for (int i = 0; i < frame.turn_command_count; ++i) {
      game_action(frame.command_len[i], frame.command[i]);
    }

    if (!loop_fast_forward(frame.turn_farthest)) {
      if (frame.turn_nearest > 1) {
        input_queue = MAX((signed) input_queue - 1, 0);
      }
    }

    if (simulationGoal <= loop_frame()) {
      loop_pause();
      continue;
    }

    for (int i = 0; i < used_apply_func; ++i) {
      functor_invoke(apply_func[i]);
    }

    for (int i = 0; i < dlfnUsedSymbols; ++i) {
      uint64_t startCall = rdtsc();
      result[i] = functor_invoke(dlfnSymbols[i].fnctor);
      uint64_t endCall = rdtsc();
      perf[i] = to_double(endCall - startCall);
    }

    for (int i = 0; i < dlfnUsedSymbols; ++i) {
      stats_sample_add(&perfStats[i], perf[i]);
    }

    for (int j = 0; j < dlfnUsedSymbols; ++j) {
      for (int i = 0; i < used_result_func; ++i) {
        result_func[i].param[0].cp = &dlfnSymbols[j];
        result_func[i].param[1].i = result[j];
        functor_invoke(result_func[i]);
      }
    }

    loop_sync();
  }
  connection_print_stats();
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
  RecordRW_t grec[MAX_PLAYER] = { 0 };

  bool multiplayer = argc > 1;
  printf("Multiplayer state %d\n", multiplayer);
  if (!multiplayer) {
    input_rw = grec;
  }
  input_rw->rec = record_alloc();

  while (!exiting) {
    game_simulation(grec);
  }

  record_free(input_rw->rec);
  input_rw->rec = NULL;
  for (int i = 0; i < MAX_PLAYER; ++i) {
    record_free(grec[i].rec);
    grec[i].rec = NULL;
  }

  return 0;
}
