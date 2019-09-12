#include "sys_network.c"

int
main(int argc, char **argv)
{
  bool configured = sys_networkConfigure("assets.rufe.org", "10123");
  bool connected = sys_networkConnect();
  static char readBuffer[4096];
  static ssize_t usedReadBuffer;

  while (!sys_networkIsReady()) {
    puts("Waiting for connection\n");
    usleep(300 * 1000);
  }

  while (configured && connected) {
    int32_t events = sys_networkPoll();

    if (events & POLLIN) {
      ssize_t bytes = sys_networkRead(sizeof(readBuffer) - usedReadBuffer,
                                      readBuffer + usedReadBuffer);
      if (bytes == -1)
        break;
      usedReadBuffer += bytes;
      printf(" poll %zu\n", usedReadBuffer);
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
