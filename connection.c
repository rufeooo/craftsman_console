#include <stdbool.h>
#include <stdio.h>

#include "macro.h"
#include "network.c"
#include "record.h"

#define MAX_PLAYER 4

enum { CONN_OK, CONN_TERM, CONN_CHANGE, CONN_STALL };

static char net_buffer[4096];
static ssize_t used_read_buffer;
static uint32_t received_bytes;
static uint32_t processed_bytes[MAX_PLAYER];
static uint32_t reads[MAX_PLAYER];
static RecordOffset_t netrec_write[MAX_PLAYER];
static RecordOffset_t netrec_read[MAX_PLAYER] = { 0 };

static int
digit_atoi(char c)
{
  return c - '0';
}

static int
fixed_atoi(char str[static 3])
{
  return digit_atoi(str[0]) * 100 + digit_atoi(str[1]) * 10
         + digit_atoi(str[2]);
}

static uint32_t
connection_buffered_max(const uint32_t player_count,
                        RecordOffset_t write_offset[static MAX_PLAYER],
                        RecordOffset_t read_offset[static MAX_PLAYER])
{
  uint32_t unread = 0;
  for (int i = 0; i < player_count; ++i) {
    uint32_t read = read_offset[i].command_count;
    uint32_t write = write_offset[i].command_count;
    unread = MAX(write - read, unread);
  }

  return unread;
}

static uint32_t
connection_buffered_min(const uint32_t player_count,
                        RecordOffset_t write_offset[static MAX_PLAYER],
                        RecordOffset_t read_offset[static MAX_PLAYER])
{
  uint32_t unread = 10; // TODO: bind to framerate
  for (int i = 0; i < player_count; ++i) {
    uint32_t read = read_offset[i].command_count;
    uint32_t write = write_offset[i].command_count;
    unread = MIN(write - read, unread);
  }

  return unread;
}

uint32_t
connection_players(Record_t *recording[static MAX_PLAYER])
{
  uint32_t count = 0;
  for (int i = 0; i < MAX_PLAYER; ++i) {
    if (!recording[i])
      continue;
    ++count;
  }

  return count;
}

int
connection_establish()
{
  bool configured = network_configure("gamehost.rufe.org", "4000");
  bool connected = network_connect();

  while (!network_ready()) {
    if (disconnected)
      return 0;

    puts("Waiting for connection");
    usleep(300 * 1000);
  }

  return 1;
}

void
connection_reset_read()
{
  for (int i = 0; i < MAX_PLAYER; ++i) {
    netrec_read[i] = (RecordOffset_t){ 0 };
  }
}

bool
connection_io()
{
  if (disconnected)
    return false;

  int32_t events = network_poll();
  if ((events & POLLOUT) == 0) {
    puts("network write unavailable\n");
    return false;
  }

  if (events & POLLIN) {
    ssize_t bytes = network_read(sizeof(net_buffer) - used_read_buffer,
                                 net_buffer + used_read_buffer);
    received_bytes += bytes;
    if (bytes == -1) {
      return false;
    }
    used_read_buffer += bytes;

    // Hangup after all bytes are drained
    if (events & POLLHUP) {
      return bytes != 0;
    }
  }

  return true;
}

bool
connection_processing(Record_t *recording[static MAX_PLAYER])
{
  while (used_read_buffer >= 8) {
    int *block_len = (int *) net_buffer;
    int length = 8 + *block_len;
    if (used_read_buffer < length)
      break;

    if (length > 9) {
      printf("Received big message %d: %s %s\n", length, &net_buffer[4],
             &net_buffer[8]);
    }
    int client_id = fixed_atoi(&net_buffer[4]);
    if (client_id >= MAX_PLAYER)
      CRASH();

    if (!recording[client_id]) {
      recording[client_id] = record_alloc();
      return false;
    }

    ++reads[client_id];
    processed_bytes[client_id] += length;
    record_append(recording[client_id], *block_len - 1, &net_buffer[8],
                  &netrec_write[client_id]);
    memmove(net_buffer, net_buffer + length, sizeof(net_buffer) - length);
    used_read_buffer -= length;
  }

  return true;
}

void
connection_queue(Record_t *recording[static MAX_PLAYER], size_t *nearest,
                 size_t *farthest)
{
  const int player_count = connection_players(recording);

  *farthest =
    connection_buffered_max(player_count, netrec_write, netrec_read);
  *nearest = connection_buffered_min(player_count, netrec_write, netrec_read);
}

int
connection_frame(Record_t *recording[static MAX_PLAYER], size_t n,
                 char buffer[n], size_t command_size[static MAX_PLAYER])
{
  const int player_count = connection_players(recording);

  for (int i = 0; i < player_count; ++i) {
    size_t cmd_len;
    const char *cmd = record_read(recording[i], &netrec_read[i], &cmd_len);
    command_size[i] = cmd_len;
    if (cmd_len > n)
      return i - 1;
    memcpy(buffer, cmd, cmd_len);
    buffer[cmd_len] = 0;
    buffer += cmd_len + 1;
    n -= cmd_len + 1;
  }

  return player_count;
}

void
connection_print_stats()
{
  puts("--net stats");
  printf("Received_bytes %d  unprocessed %zu\n", received_bytes,
         used_read_buffer);
  for (int i = 0; i < MAX_PLAYER; ++i) {
    printf("player %d: %d reads %d bytes\n", i, reads[i], processed_bytes[i]);
  }
}

int
connection_sync(Record_t *recording[static MAX_PLAYER])
{
  if (!connection_io()) {
    return CONN_TERM;
  }

  if (!connection_processing(recording)) {
    return CONN_CHANGE;
  }

  return CONN_OK;
}
