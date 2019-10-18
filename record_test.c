
#include <stdio.h>

#include "macro.h"
#include "record.h"

static Record_t *recording;

void
input_handler(size_t strlen, char *str)
{
  record_append(recording, strlen, str);

  printf("%s\n", str);
}

int
main(int argc, char **argv)
{
  recording = record_alloc();

  record_append(recording, 1, "a");
  record_append(recording, 1, "b");
  record_append(recording, 1, "c");
  record_append(recording, 1, "d");

  record_debug(recording);
  Record_t *copyRec = record_clone(recording);
  record_debug(copyRec);
  record_seek_write(recording, 0u);

  record_playback_all(copyRec, input_handler);
  record_debug(recording);
  record_debug(copyRec);

  record_seek_write(recording, 0u);
  record_seek_read(copyRec, 0u);
  while (record_playback(copyRec, input_handler)) {
  }

  record_debug(recording);
  record_debug(copyRec);

  record_seek_read(copyRec, 0u);
  while (record_playback(copyRec, input_handler)) {
  }

  record_debug(recording);
  record_debug(copyRec);

  record_free(copyRec);
  record_free(recording);
}
