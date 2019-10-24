#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dlfn.h"
#include "float.h"
#include "functor.h"
#include "hash.h"
#include "input.h"
#include "loop.c"
#include "macro.h"
#include "network.c"
#include "notify.h"
#include "record.h"
#include "stats.h"

#define MAX_PLAYER 4
#define MAX_FUNC 128

static uint32_t writes;
static uint32_t reads[MAX_PLAYER];
static uint32_t readBytes[MAX_PLAYER];
static const char *dlpath = "code/feature.so";
static const uint32_t simulationMax = UINT32_MAX;
static uint32_t simulationGoal;
static bool exiting;
static Record_t *irec;
static RecordOffset_t irec_write;
static Record_t *netrec[MAX_PLAYER];
static RecordOffset_t netrec_write[MAX_PLAYER];
static Functor_t apply_func[MAX_FUNC];
static size_t used_apply_func;
static Functor_t result_func[MAX_FUNC];
static size_t used_result_func;

size_t
print_result(const Symbol_t *sym, size_t r)
{
  printf("0x%zX %s 0x%zX 0x%zX 0x%zX\n", r, sym->name, sym->fnctor.param[0].i,
         sym->fnctor.param[1].i, sym->fnctor.param[2].i);
  return 0;
}

void
print_players()
{
  int count = 0;
  for (int i = 0; i < MAX_PLAYER; ++i) {
    if (!netrec[i])
      continue;
    ++count;
  }
  printf("Player Count: %d.\n", count);
}

void
prompt()
{
  dlfn_print_symbols();
  printf("Simulation will run until frame %d.\n", simulationGoal);
  print_players();
  puts("(q)uit (s)imulation (b)enchmark (a)pply (p)rint (h)ash (o)bject "
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
  uint64_t hash_seed = 0;
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
  }

  record_append(irec, len, input, &irec_write);
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

  ++writes;
  if (!len) {
    network_write(1, "");
    return;
  }

  printf("network write %zu\n", len);
  network_write(len + 1, input);
}

void
network_to_game(size_t len, char *input)
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
    while (used_read_buffer >= 8) {
      int *block_len = (int *) net_buffer;
      int length = 8 + *block_len;
      if (used_read_buffer >= length) {
        if (length > 9) {
          printf("Received big message %d: %s %s\n", length, &net_buffer[4],
                 &net_buffer[8]);
        }
        int client_id = fixed_atoi(&net_buffer[4]);
        if (client_id >= MAX_PLAYER)
          CRASH();

        if (!netrec[client_id]) {
          netrec[client_id] = record_alloc();
          loop_halt();
        }

        ++reads[client_id];
        readBytes[client_id] += length;
        record_append(netrec[client_id], *block_len - 1, &net_buffer[8],
                      &netrec_write[client_id]);
      }
      memmove(net_buffer, net_buffer + length, sizeof(net_buffer) - length);
      used_read_buffer -= length;
    }
  }

  return true;
}

uint32_t
network_buffered(RecordOffset_t write_offset[static MAX_PLAYER],
                 RecordOffset_t read_offset[static MAX_PLAYER])
{
  size_t unread = 0;
  for (int i = 0; i < MAX_PLAYER; ++i) {
    uint32_t read = read_offset[i].command_count;
    uint32_t write = write_offset[i].command_count;
    unread = MAX(write - read, unread);
  }

  return unread;
}

bool
network_input_ready(Record_t *player_input[static MAX_PLAYER],
                    RecordOffset_t read_offset[static MAX_PLAYER])
{
  for (int i = 0; i < MAX_PLAYER; ++i) {
    if (!player_input[i])
      continue;
    if (record_can_playback(player_input[i], &read_offset[i]))
      continue;
    return false;
  }

  return true;
}

void
game_simulation()
{
  const uint32_t FF_THRESHOLD = 10;
  RecordOffset_t inputRead = { 0 };
  RecordOffset_t netrec_read[MAX_PLAYER] = { 0 };
  char *watchDirs[] = { "code" };
  size_t result[MAX_SYMBOLS];
  double perf[MAX_SYMBOLS];
  Stats_t perfStats[MAX_SYMBOLS];
  for (int i = 0; i < MAX_SYMBOLS; ++i) {
    stats_init(&perfStats[i]);
  }
  memset(apply_func, 0, sizeof(apply_func));
  used_apply_func = 0;
  memset(result_func, 0, sizeof(result_func));
  used_result_func = 0;
  record_reset(irec);
  irec_write = (RecordOffset_t){ 0 };

  loop_init(10);
  loop_print_status();
  notify_init(IN_CLOSE_WRITE, ARRAY_LENGTH(watchDirs), watchDirs);
  dlfn_init(dlpath);
  dlfn_open();
  simulationGoal = 0;
  input_init();
  prompt();
  while (loop_run()) {
    // printf("%d frame %d pause %d stall\n", frame, pauseFrame, stallFrame);

    notify_poll(notify_callback);
    input_poll(input_callback);

    while (!record_playback(irec, input_to_network, &inputRead)) {
      if (frame + pauseFrame + FF_THRESHOLD < writes) {
        // puts("write suspension");
        break;
      }

      record_append(irec, 0, 0, &irec_write);
    }

    if (!network_io()) {
      puts("Network failure.");
      loop_halt();
      continue;
    }

    if (!network_input_ready(netrec, netrec_read)) {
      printf(".");
      fflush(stdout);
      loop_stall();
      continue;
    }

    // printf("Writes %d skew %d\n", writes, writes - (frame + pauseFrame));
    for (int i = 0; i < MAX_PLAYER; ++i) {
      if (!netrec[i])
        continue;
      record_playback(netrec[i], network_to_game, &netrec_read[i]);

      // printf("%d: %d reads %d readBytes\n", i, reads[i], readBytes[i]);
    }

    size_t pending = network_buffered(netrec_write, netrec_read);
    printf("%zu pending command\n", pending);
    loop_adjustment(pending > FF_THRESHOLD);

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
    if (disconnected) {
      puts("Network unable to connect.");
      return 1;
    }

    puts("Waiting for connection %d %d\n");
    usleep(300 * 1000);
  }

  irec = record_alloc();
  while (!exiting) {
    game_simulation();
  }
  for (int i = 0; i < MAX_PLAYER; ++i) {
    record_free(netrec[i]);
  }
  record_free(irec);

  return 0;
}
