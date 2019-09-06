#include "sys_notify.c"
#include <stdio.h>

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

  sys_notifyInit(argc - 1, argv + 1);
  printf("%d\n", sys_notifyLastError());

  while (true) {
    if (!sys_notifyPoll(change_event))
      printf("lastError %d\n", sys_notifyLastError());
  }

  sys_notifyShutdown();

  return 0;
}
