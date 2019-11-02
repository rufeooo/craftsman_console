#include <stdbool.h>
#include <stdio.h>

#include "macro.h"
#include "network.c"
#include "record.h"

#define MAX_PLAYER 4

enum { CONN_OK, CONN_TERM, CONN_CHANGE, CONN_STALL };

static char connection_receive_buffer[4096];
static ssize_t used_receive_buffer;
static uint32_t bytes_received;
static uint32_t bytes_processed[MAX_PLAYER];
static uint32_t messages_processed[MAX_PLAYER];

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
    ssize_t bytes =
      network_read(sizeof(connection_receive_buffer) - used_receive_buffer,
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
    record_append(recording[client_id].rec, *block_len - 1,
                  &connection_receive_buffer[8], &recording[client_id].write);
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
  printf("Received_bytes %d  unprocessed %zu\n", bytes_received,
         used_receive_buffer);
  for (int i = 0; i < MAX_PLAYER; ++i) {
    printf("player %d: %d messages_processed %d bytes\n", i,
           messages_processed[i], bytes_processed[i]);
  }
}

int
connection_sync(RecordRW_t recording[static MAX_PLAYER])
{
  if (!connection_io()) {
    return CONN_TERM;
  }

  if (!connection_processing(recording)) {
    return CONN_CHANGE;
  }

  return CONN_OK;
}
