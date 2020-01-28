#include <stdio.h>
#include <string.h>
#include <sys/poll.h>
#include <unistd.h>

#include "macro.h"
#include "network.cc"

int
main(int argc, char **argv)
{
  EndPoint_t client_ep;
  bool configured = network_configure(&client_ep, "gamehost.rufe.org", "4000");
  bool connected = network_connect(&client_ep);
  static char readBuffer[4096];
  static uint64_t used_read_buffer;

  while (!network_ready(&client_ep)) {
    puts("Waiting for connection\n");
    usleep(300 * 1000);
  }

  while (configured && connected) {
    int32_t events = network_poll(&client_ep, POLLOUT | POLLIN | POLLERR, 0);

    if (FLAGGED(events, POLLIN)) {
      ssize_t bytes =
          network_read(client_ep.sfd, sizeof(readBuffer) - used_read_buffer,
                       readBuffer + used_read_buffer);
      if (bytes == -1) break;
      used_read_buffer += bytes;
      printf(" poll %zu\n", used_read_buffer);
    }

    if (events & POLLOUT) {
      puts("TODO write\n");
    }

    if (!events) {
      puts("tick\n");
      usleep(300 * 1000);
    }
  }
}
