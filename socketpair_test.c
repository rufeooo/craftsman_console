#include "input.c"
#include "network.c"
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
  EndPoint_t server_ep;

  input_init();
  if (!network_socketpair(&client_ep, &server_ep))
    return 1;

  server_init(&server_ep);
  while (!exiting) {
    input_poll(input_event);
    int revents = network_poll(&client_ep, POLLOUT | POLLIN | POLLERR, 0);
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
  puts("client loop term");

  input_shutdown();
  close(client_ep.sfd);
  close(server_ep.sfd);

  server_term();

  return 0;
}

