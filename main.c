#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "connection.c"
#include "dlfn.h"
#include "execute.c"
#include "float.h"
#include "functor.h"
#include "hash.h"
#include "input.h"
#include "loop.c"
#include "macro.h"
#include "notify.h"
#include "record.h"
#include "stats.h"

static bool buffering = false;
static const char *dlpath = "code/feature.so";
static uint32_t simulation_goal;
static bool exiting;
static RecordRW_t *input_rw;

typedef struct {
  uint32_t turn_command_count;
  char *command[MAX_PLAYER];
  size_t command_len[MAX_PLAYER];
} CommandFrame_t;

typedef struct {
  uint32_t turn_nearest;
  uint32_t turn_farthest;
} CommandPreview_t;

void
build_command_preview(size_t player_count, RecordRW_t recording[player_count],
                      CommandPreview_t *out_state)
{
  CommandPreview_t ret_state = { .turn_nearest = loop_input_queue_max(),
                                 .turn_farthest = 0 };
  for (int i = 0; i < player_count; ++i) {
    uint32_t rcmd = recording[i].read.command_count;
    uint32_t wcmd = recording[i].write.command_count;

    ret_state.turn_farthest = MAX(wcmd - rcmd, ret_state.turn_farthest);
    ret_state.turn_nearest = MIN(wcmd - rcmd, ret_state.turn_nearest);
  }

  *out_state = ret_state;
}

#define FRAME_BUF_LEN 4096
bool
build_command_frame(size_t player_count, RecordRW_t recording[player_count],
                    CommandFrame_t *out_state)
{
  static char buffer[FRAME_BUF_LEN];
  CommandFrame_t ret_state = { .turn_command_count = player_count };
  char *buffer_write = buffer;
  char *buffer_end = buffer + FRAME_BUF_LEN;

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

typedef void (*GameInputFn)(RecordRW_t game_record[static MAX_PLAYER]);

void
noop(RecordRW_t game_record[static MAX_PLAYER])
{
}

void
multiplayer_gi(RecordRW_t game_record[static MAX_PLAYER])
{
  int status = connection_sync(game_record);

  const int target = loop_write_frame();
  while (target > input_rw->read.command_count) {
    size_t cmd_len;
    const char *cmd = record_read(input_rw->rec, &input_rw->read, &cmd_len);
    ++cmd_len;
    ssize_t written = network_write(cmd_len, cmd);
    buffering = buffering | (written != cmd_len);
  }

  switch (status) {
  case CONN_TERM:
    puts("Network failure.");
    loop_halt();
    exiting = true;
    return;
  case CONN_CHANGE:
    loop_halt();
    return;
  };
}

void
prompt(int player_count)
{
  dlfn_print_symbols();
  printf("Simulation will run until frame %d.\n", simulation_goal);
  printf("Player Count: %d\n", player_count);
  puts("(q)uit (i)nfo (s)imulation (b)enchmark (a)pply (p)rint (h)ash "
       "(o)bject "
       "(r)eload>");
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
input_callback(size_t len, char *input)
{
  // These events are not recorded
  switch (input[0]) {
  case 'r':
    simulation_goal = 0;
    loop_halt();
    return;
  case 'q':
    simulation_goal = 0;
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
    simulation_goal = execute_simulation(len, input);
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
notify_callback(int idx, const struct inotify_event *event)
{
  printf("File change %s\n", event->name);
  if (!strstr(dlpath, event->name))
    return;

  simulation_goal = 0;
  loop_halt();
}

void
game_simulation(RecordRW_t game_record[static MAX_PLAYER], GameInputFn gifn)
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

  loop_init(10);
  loop_print_status();
  notify_init(IN_CLOSE_WRITE, ARRAY_LENGTH(watchDirs), watchDirs);
  dlfn_init(dlpath);
  dlfn_open();
  simulation_goal = 0;
  input_init();
  prompt(player_count);
  while (loop_run()) {
    loop_print_frame();

    notify_poll(notify_callback);
    input_poll(input_callback);

    const int target = loop_write_frame();
    while (target > input_rw->write.command_count) {
      record_append(input_rw->rec, 0, 0, &input_rw->write);
    }

    gifn(game_record);

    CommandPreview_t preview;
    build_command_preview(player_count, game_record, &preview);
    if (!preview.turn_nearest) {
      printf("+");
      printf("[ %d farthest ] [ %d nearest ] \n", preview.turn_farthest,
             preview.turn_nearest);
      fflush(stdout);
      input_queue = MIN(input_queue + 1, input_queue_max);
      loop_stall();
      continue;
    }
    stats_sample_add(&net_perf,
                     to_double(loop_input_queue() - preview.turn_nearest));
    /*printf("(%5.2e, %5.2e) range\t%5.2e mean ± %4.02f%%\t\n",
           stats_min(&net_perf), stats_max(&net_perf),
       stats_mean(&net_perf), 100.0 * stats_rs_dev(&net_perf));*/

    if (!loop_fast_forward(preview.turn_farthest)) {
      if (preview.turn_nearest > 1) {
        input_queue = MAX((signed) input_queue - 1, 0);
      }
    }

    CommandFrame_t frame;
    if (!build_command_frame(player_count, game_record, &frame))
      CRASH();

    for (int i = 0; i < frame.turn_command_count; ++i) {
      game_action(frame.command_len[i], frame.command[i]);
    }

    if (simulation_goal <= loop_frame()) {
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
  RecordRW_t grec[MAX_PLAYER] = { 0 };
  RecordRW_t mp_input = { 0 };
  GameInputFn gi_interface;

  bool multiplayer = argc > 1;
  printf("Multiplayer state %d\n", multiplayer);
  if (multiplayer) {
    gi_interface = multiplayer_gi;
    input_rw = &mp_input;
    if (!connection_establish()) {
      puts("Failed to connect");
      return 1;
    }
  } else {
    gi_interface = noop;
    input_rw = grec;
    input_queue = 0;
  }
  input_rw->rec = record_alloc();

  while (!exiting) {
    game_simulation(grec, gi_interface);
    for (int i = 0; i < MAX_PLAYER; ++i) {
      grec[i].read = (RecordOffset_t){ 0 };
    }
  }

  record_free(input_rw->rec);
  input_rw->rec = NULL;
  for (int i = 0; i < MAX_PLAYER; ++i) {
    record_free(grec[i].rec);
  }

  if (multiplayer)
    connection_print_stats();

  return 0;
}
