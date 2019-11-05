#pragma once

#include <stdbool.h>
#include <stdint.h>

#define SS_STORAGE 128

typedef struct {
  int sfd;
  char storage[SS_STORAGE];
  uint32_t used_storage;
  bool connected;
  bool connecting;
  bool disconnected;
} EndPoint_t;

void endpoint_init(EndPoint_t *ep);
void endpoint_from_fd(int fd, EndPoint_t *ep);
void endpoint_term(EndPoint_t *ep);

bool network_configure(EndPoint_t *ep, const char *host,
                       const char *service_or_port);
bool network_socketpair(EndPoint_t *client_ep, EndPoint_t *server_ep);
bool network_connect(EndPoint_t *ep);
int64_t network_read(int fd, int64_t n, char buffer[n]);
int64_t network_write(int fd, int64_t n, const char buffer[n]);
int32_t network_poll(EndPoint_t *ep);
bool network_ready(EndPoint_t *ep);

