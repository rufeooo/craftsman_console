#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "macro.h"
#include "network.h"

static pthread_attr_t attr;
static pthread_t thread;

static void
server_message(EndPoint_t *ep, uint32_t n, char message[static n + 1])
{
  const char player[] = "000";

  ++n; // nullterm
  int written = 0;
  written += network_write(ep->sfd, sizeof(n), (const char *) &n);
  written += network_write(ep->sfd, sizeof(player), player);
  written += network_write(ep->sfd, n, message);
}

static uint32_t
server_process(EndPoint_t *ep, uint32_t n, char bytes[n])
{
  int msg_start = 0;
  for (int i = 0; i < n; ++i) {
    if (bytes[i] == 0) {
      server_message(ep, i - msg_start, &bytes[msg_start]);
      msg_start = i + 1;
    }
  }

  return msg_start;
}

static bool
errno_would_block()
{
  return (errno == EAGAIN || errno == EWOULDBLOCK);
}

static bool
server_halt(ssize_t bytes, bool pollhup)
{
  switch (bytes) {
  case -1:
    return !errno_would_block();
  case 0:
    return pollhup;
  }

  return false;
}

#define SERVER_RECV_MAX 4096
static void *
server_routine(void *arg)
{
  static char receive_buffer[SERVER_RECV_MAX];
  static uint32_t used_receive_buffer;
  static uint64_t bytes_received;
  EndPoint_t ep = *(EndPoint_t *) arg;

  while (!ep.disconnected) {
    int32_t events = network_poll(&ep);
    if (UNFLAGGED(events, POLLOUT)) {
      puts("network_server stop: write unavailable");
      break;
    }

    if (FLAGGED(events, POLLIN)) {
      ssize_t bytes =
        network_read(ep.sfd, sizeof(receive_buffer) - used_receive_buffer,
                     receive_buffer + used_receive_buffer);

      // Hangup after all bytes are drained
      if (server_halt(bytes, FLAGGED(events, POLLHUP))) {
        printf("errno %d\n", errno);
        puts("network_server drain: all bytes read");
        break;
      }

      bytes_received += bytes;
      used_receive_buffer += bytes;

      ssize_t processed_bytes =
        server_process(&ep, used_receive_buffer, receive_buffer);
      if (processed_bytes) {
        memmove(receive_buffer, &receive_buffer[processed_bytes],
                used_receive_buffer - processed_bytes);
        used_receive_buffer -= processed_bytes;
      }
    }

    usleep(100 * 1000);
  }

  printf("network_server exiting %d\n", ep.disconnected);
  return 0;
}

void
server_init(EndPoint_t *ep)
{
  static EndPoint_t init_ep;
  if (thread)
    return;

  init_ep = *ep;
  pthread_attr_init(&attr);
  pthread_create(&thread, &attr, server_routine, &init_ep);
}

void
server_term()
{
  if (!thread)
    return;

  void *thread_ret;
  pthread_join(thread, &thread_ret);
  printf("thread returned: %p\n", thread_ret);
  pthread_attr_destroy(&attr);
}

