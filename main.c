#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dlfn.h"
#include "float.h"
#include "functor.h"
#include "input.h"
#include "loop.c"
#include "macro.h"
#include "network.c"
#include "notify.h"
#include "record.h"
#include "stats.h"

#define MAX_PLAYER 4

static int writes;
static int reads;
static int readBytes;
static const char *dlpath = "code/feature.so";
static const uint32_t simulationMax = UINT32_MAX;
static uint32_t simulationGoal;
static bool exiting;
static Record_t *recording;
static Record_t *from_network[MAX_PLAYER];
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
  case 'r':
    simulationGoal = 0;
    loop_halt();
    return;
  case 'q':
    simulationGoal = 0;
    exiting = true;
    loop_halt();
    return;
  }

  record_append(recording, len, input);
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
  }
}

void
input_to_network(size_t len, char *input)
{
  if (input[len] != 0)
    CRASH();

  ++writes;
  if (!len) {
    network_write(1, "");
    return;
  }

  printf("network write %zu\n", len);
  network_write(len + 1, input);
}

void
game_input(size_t len, char *input)
{
  static char buffer[4096];

  memcpy(buffer, input, len);
  buffer[len] = 0;

  game_action(len, buffer);
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

int
digit_atoi(char c)
{
  return c - '0';
}

int
fixed_atoi(char str[static 3])
{
  return digit_atoi(str[0]) * 100 + digit_atoi(str[1]) * 10
         + digit_atoi(str[2]);
}

bool
network_io()
{
  int32_t events = network_poll();
  if ((events & POLLOUT) == 0) {
    puts("network write unavailable\n");
    return false;
  }

  if (events & POLLIN) {
    static char net_buffer[4096];
    static ssize_t used_read_buffer;
    ssize_t bytes = network_read(sizeof(net_buffer) - used_read_buffer,
                                 net_buffer + used_read_buffer);
    if (bytes == -1) {
      return false;
    }
    used_read_buffer += bytes;
    while (used_read_buffer >= 4) {
      int *header = (int *) net_buffer;
      int length = 4 + *header;
      if (used_read_buffer >= length) {
        if (length > 9) {
          printf("Received big message %d: %s\n", length, &net_buffer[4]);
        }
        int client_id = fixed_atoi(&net_buffer[4]);
        if (client_id < MAX_PLAYER) {
          if (!from_network[client_id]) {
            from_network[client_id] = record_alloc();
          }

          ++reads;
          readBytes += length;
          record_append(from_network[client_id], *header - 5, &net_buffer[8]);
        }
      }
      memmove(net_buffer, net_buffer + length, sizeof(net_buffer) - length);
      used_read_buffer -= length;
    }
  }

  return true;
}

bool
network_input_ready(Record_t *player_input[static MAX_PLAYER],
                    int read_offset[static MAX_PLAYER])
{
  for (int i = 0; i < MAX_PLAYER; ++i) {
    if (!player_input[i])
      continue;
    if (record_can_playback(player_input[i], read_offset[i]))
      continue;
    return false;
  }

  return true;
}

void
game_simulation()
{
  int inputRead = 0;
  int networkRead[MAX_PLAYER] = { 0 };
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
  input_init();
  prompt();
  while (loop_run()) {
    input_poll(input_callback);

    while (!record_playback(recording, input_to_network, &inputRead)) {
      record_append(recording, 0, 0);
    }
    notify_poll(notify_callback);

    if (!network_io()) {
      puts("Network failure.");
      loop_halt();
      continue;
    }

    printf("%d writes %d reads %d readBytes\n", writes, reads, readBytes);

    if (!network_input_ready(from_network, networkRead)) {
      printf(".");
      loop_stall();
      continue;
    }

    for (int i = 0; i < MAX_PLAYER; ++i) {
      if (!from_network[i])
        continue;
      record_playback(from_network[i], game_input, &networkRead[i]);
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
  bool configured = network_configure("gamehost.rufe.org", "4000");
  bool connected = network_connect();

  while (!network_ready()) {
    puts("Waiting for connection\n");
    usleep(300 * 1000);
  }

  recording = record_alloc();
  while (!exiting) {
    game_simulation();
  }
  for (int i = 0; i < MAX_PLAYER; ++i) {
    record_free(from_network[i]);
  }
  record_free(recording);

  return 0;
}
