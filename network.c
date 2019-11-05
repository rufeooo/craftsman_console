#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <stdio.h>

#include "network.h"

static void
network_no_nagle(int fd)
{
  if (fd <= STDERR_FILENO)
    return;

  int flag = 1;
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
}

static void
network_non_blocking(int fd)
{
  int flags = 0;
  if (fd <= STDERR_FILENO)
    return;

  flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1)
    return;

  printf("flags %u\n", flags);

  fcntl(fd, F_SETFL, flags | O_NONBLOCK);

  flags = fcntl(fd, F_GETFL, 0);
  printf("new flags %u\n", flags);
}

void
endpoint_init(EndPoint_t *ep)
{
  static_assert(_SS_SIZE == SS_STORAGE,
                "EndPoint_t: System _SS_SIZE != SS_STORAGE");
  *ep = (EndPoint_t){ 0 };
}

void
endpoint_from_fd(int fd, EndPoint_t *ep)
{
  *ep = (EndPoint_t){ .sfd = fd };
}

void
endpoint_term(EndPoint_t *ep)
{
  if (ep->sfd > STDERR_FILENO)
    close(ep->sfd);
  ep->sfd = 0;
}

bool
network_configure(EndPoint_t *ep, const char *host,
                  const char *service_or_port)
{
  static struct addrinfo hints;
  struct addrinfo *result = NULL;

  endpoint_init(ep);

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = 0;
  hints.ai_protocol = IPPROTO_TCP;

  getaddrinfo(host, service_or_port, &hints, &result);
  if (!result)
    return false;

  ep->sfd =
    socket(result->ai_family, result->ai_socktype, result->ai_protocol);
  network_no_nagle(ep->sfd);
  network_non_blocking(ep->sfd);

  if (ep->sfd == -1)
    return false;

  memcpy(ep->storage, result->ai_addr, result->ai_addrlen);
  ep->used_storage = result->ai_addrlen;

  freeaddrinfo(result);

  return true;
}

bool
network_socketpair(EndPoint_t *client_ep, EndPoint_t *server_ep)
{
  int sv[2];
  int ret = socketpair(AF_LOCAL, SOCK_STREAM | SOCK_NONBLOCK, 0, sv);

  if (ret) {
    printf("Failed to form socketpair, errno %d\n", errno);
    return false;
  }

  endpoint_from_fd(sv[0], client_ep);
  endpoint_from_fd(sv[1], server_ep);

  return true;
}

static bool
network_interrupted()
{
  return (errno == EINTR || errno == EINPROGRESS);
}

bool
network_connect(EndPoint_t *ep)
{
  if (ep->sfd <= STDERR_FILENO)
    return false;

  int result =
    connect(ep->sfd, (struct sockaddr *) ep->storage, ep->used_storage);
  if (result == -1 && !network_interrupted()) {
    return false;
  }

  ep->connecting = true;

  return true;
}

int64_t
network_read(int fd, int64_t n, char buffer[n])
{
  return read(fd, buffer, n);
}

int64_t
network_write(int fd, int64_t n, const char buffer[n])
{
  return write(fd, buffer, n);
}

int32_t
network_poll(EndPoint_t *ep)
{
  struct pollfd fds = { .fd = ep->sfd, .events = POLLIN | POLLOUT | POLLERR };
  int poll_num = poll(&fds, 1, 0);

  if (poll_num == -1) {
    if (errno == EINTR)
      return 0;

    return errno;
  }

  if (fds.revents & (POLLIN | POLLOUT))
    ep->connected = ep->connecting;
  if (fds.revents & POLLERR)
    ep->disconnected = true;

  // printf("%d poll_num %d revents\n", poll_num, fds.revents);

  return fds.revents;
}

bool
network_ready(EndPoint_t *ep)
{
  network_poll(ep);

  return ep->connected && !ep->disconnected;
}

