#include <stdio.h>

#include "notify.h"

void
change_event(int idx, const struct inotify_event *ev)
{
  printf("\nchange_event %d %d %s\n", idx, ev->mask, ev->name);
  if (ev->mask & IN_OPEN)
    printf("IN_OPEN: ");
  if (ev->mask & IN_DELETE_SELF)
    printf("IN_DELETE_SELF: ");
  if (ev->mask & IN_DELETE)
    printf("IN_DELETE: ");
  if (ev->mask & IN_CLOSE_NOWRITE)
    printf("IN_CLOSE_NOWRITE: ");
  if (ev->mask & IN_CLOSE_WRITE)
    printf("IN_CLOSE_WRITE: ");
  if (ev->mask & IN_IGNORED)
    printf("IN_IGNORED: ");
}

int
main(int argc, char **argv)
{
  if (argc <= 1) {
    puts("Usage: a.out <watchlist>\n");
    return 1;
  }

  notify_init(IN_CLOSE_WRITE, argc - 1, argv + 1);
  printf("%d\n", notify_last_error());

  while (true) {
    if (!notify_poll(change_event))
      printf("lastError %d\n", notify_last_error());
  }

  notify_shutdown();

  return 0;
}
