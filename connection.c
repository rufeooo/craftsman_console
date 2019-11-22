#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "macro.h"
#include "network.h"
#include "network_server.c"
#include "record.h"

#define MAX_PLAYER 4

enum { CONN_OK, CONN_TERM, CONN_CHANGE };

static char connection_receive_buffer[4096];
static ssize_t used_receive_buffer;
static uint32_t bytes_received;
static uint32_t bytes_processed[MAX_PLAYER];
static uint32_t messages_processed[MAX_PLAYER];
static bool buffering = false;
static EndPoint_t client_ep;
static EndPoint_t server_ep;

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

int
connection_init(const char *host)
{
  if (!host) {
    network_socketpair(&client_ep, &server_ep);
    server_init(&server_ep);
    return 1;
  }

  bool configured = network_configure(&client_ep, host, "4000");
  bool connected = network_connect(&client_ep);

  while (!network_ready(&client_ep)) {
    if (client_ep.disconnected)
      return 0;

    puts("Waiting for connection");
    usleep(300 * 1000);
  }

  return 1;
}

bool
connection_io()
{
  if (client_ep.disconnected)
    return false;

  int32_t events = network_poll(&client_ep, POLLIN | POLLOUT | POLLERR, 0);
  if ((events & POLLOUT) == 0) {
    puts("network write unavailable\n");
    return false;
  }

  if (events & POLLIN) {
    ssize_t bytes = network_read(
      client_ep.sfd, sizeof(connection_receive_buffer) - used_receive_buffer,
      connection_receive_buffer + used_receive_buffer);
    bytes_received += bytes;
    if (bytes == -1) {
      return false;
    }
    used_receive_buffer += bytes;

    // Hangup after all bytes are drained
    if (events & POLLHUP) {
      return bytes != 0;
    }
  }

  return true;
}

bool
connection_processing(RecordRW_t recording[static MAX_PLAYER])
{
  while (used_receive_buffer >= 8) {
    int *block_len = (int *) connection_receive_buffer;
    int length = 8 + *block_len;
    if (used_receive_buffer < length)
      break;

    if (length > 9) {
      printf("Received big message %d: %s %s\n", length,
             &connection_receive_buffer[4], &connection_receive_buffer[8]);
    }
    int client_id = fixed_atoi(&connection_receive_buffer[4]);
    if (client_id >= MAX_PLAYER)
      CRASH();

    if (!recording[client_id].rec) {
      recording[client_id].rec = record_alloc();
      return false;
    }

    ++messages_processed[client_id];
    bytes_processed[client_id] += length;
    record_append(*block_len - 1, &connection_receive_buffer[8],
                  recording[client_id].rec, &recording[client_id].write);
    memmove(connection_receive_buffer, connection_receive_buffer + length,
            sizeof(connection_receive_buffer) - length);
    used_receive_buffer -= length;
  }

  return true;
}

void
connection_print_stats()
{
  puts("--net stats");
  printf(
    "[ Received_bytes %d ]  [ unprocessed %zu ] [write buffering state %d]\n",
    bytes_received, used_receive_buffer, buffering);
  for (int i = 0; i < MAX_PLAYER; ++i) {
    printf("player %d: %d messages_processed %d bytes\n", i,
           messages_processed[i], bytes_processed[i]);
  }
}

typedef int (*NetworkSync_t)(uint32_t target_frame, RecordRW_t *input,
                             RecordRW_t game_record[static MAX_PLAYER]);

int
connection_sync(uint32_t target_frame, RecordRW_t *input,
                RecordRW_t game_record[static MAX_PLAYER])

{
  if (!connection_io()) {
    puts("Network failure.");
    return CONN_TERM;
  }

  if (!connection_processing(game_record)) {
    return CONN_CHANGE;
  }

  while (target_frame > input->read.command_count) {
    size_t cmd_len;
    const char *cmd = record_read(input->rec, &input->read, &cmd_len);
    ++cmd_len;
    ssize_t written = network_write(client_ep.sfd, cmd_len, cmd);
    buffering = buffering | (written != cmd_len);
  }

  return CONN_OK;
}

void
connection_term()
{
  endpoint_term(&client_ep);
  endpoint_term(&server_ep);
  server_term();
}

