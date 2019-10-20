#include "network.c"

int
main(int argc, char **argv)
{
  bool configured = network_configure("gamehost.rufe.org", "4000");
  bool connected = network_connect();
  static char readBuffer[4096];
  static ssize_t used_read_buffer;

  while (!network_ready()) {
    puts("Waiting for connection\n");
    usleep(300 * 1000);
  }

  while (configured && connected) {
    int32_t events = network_poll();

    if (events & POLLIN) {
      ssize_t bytes = network_read(sizeof(readBuffer) - used_read_buffer,
                                   readBuffer + used_read_buffer);
      if (bytes == -1)
        break;
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
