#include "input.c"
#include "network_server.c"

static bool exiting;
static EndPoint_t client_ep;

void
input_event(size_t strlen, char *str)
{
  switch (*str) {
  case 'q':
    exiting = true;
    return;
  }

  ++strlen;
  int written = network_write(client_ep.sfd, strlen, str);
  printf("%zd strlen %d written\n", strlen, written);
}

int
main(int argc, char **argv)
{
  input_init();
  int sv[2];
  int ret = socketpair(AF_LOCAL, SOCK_STREAM | SOCK_NONBLOCK, 0, sv);

  if (ret)
    return errno;

  network_serve(&sv[1]);
  endpoint_init(&client_ep);
  client_ep.sfd = sv[0];
  while (!exiting) {
    input_poll(input_event);
  }

  input_shutdown();
  close(sv[0]);
  close(sv[1]);

  network_serve_wait_for_exit();

  return 0;
}

