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
  printf("%zd strlen %d client written\n", strlen, written);
}

int
main(int argc, char **argv)
{
  input_init();
  int sv[2];
  int ret = socketpair(AF_LOCAL, SOCK_STREAM | SOCK_NONBLOCK, 0, sv);

  if (ret)
    return errno;

  server_init(&sv[1]);
  endpoint_from_fd(sv[0], &client_ep);
  while (!exiting) {
    input_poll(input_event);
    int revents = network_poll(&client_ep);
    if (FLAGGED(revents, POLLIN)) {
      static uint32_t used_receive_buffer;
      static char receive_buffer[4096];
      const uint32_t header_bytes = 8;
      int bytes_read = network_read(
        client_ep.sfd, sizeof(receive_buffer) - used_receive_buffer,
        &receive_buffer[used_receive_buffer]);
      printf("client bytes_read %d\n", bytes_read);
      if (bytes_read == 0 && FLAGGED(revents, POLLHUP)) {
        puts("client pollhup");
        break;
      }
      used_receive_buffer += bytes_read;
      if (used_receive_buffer < header_bytes) {
        continue;
      }

      uint32_t *block_len = (uint32_t *) receive_buffer;
      if (*block_len > sizeof(receive_buffer) - header_bytes) {
        printf("client buffer exhausted on block_len %u\n", *block_len);
        break;
      }
      if (used_receive_buffer < *block_len + header_bytes) {
        continue;
      }

      printf("Client %u received: %s\n", used_receive_buffer,
             &receive_buffer[header_bytes]);
      int remaining_buffer = used_receive_buffer - header_bytes - *block_len;
      memmove(receive_buffer, &receive_buffer[header_bytes + *block_len],
              remaining_buffer);
      used_receive_buffer = remaining_buffer;
    }
  }

  input_shutdown();
  close(sv[0]);
  close(sv[1]);

  server_term();

  return 0;
}

