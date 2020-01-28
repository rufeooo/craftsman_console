
#include <stdio.h>

#include "macro.h"
#include "record.cc"

static Record_t *recording;
static RecordOffset_t write_offset;

void
input_handler(size_t strlen, char *str)
{
  record_append(strlen, str, recording, &write_offset);

  printf("%s\n", str);
}

void
test_handler(size_t strlen, char *str)
{
  printf("ok %zd\n", strlen);
}

int
main(int argc, char **argv)
{
  recording = record_alloc();

  record_append(0, 0, recording, &write_offset);
  RecordOffset_t readOffset = {0};
  record_playback(recording, test_handler, &readOffset);

  record_append(1, "a", recording, &write_offset);
  record_append(1, "b", recording, &write_offset);
  record_append(1, "c", recording, &write_offset);
  record_append(1, "d", recording, &write_offset);

  record_debug(recording);
  Record_t *copy = record_clone(recording);
  record_debug(copy);
  if (record_compare(recording, copy)) {
    puts("clone not equal!");
  }
  write_offset = (RecordOffset_t){0};

  record_playback_all(copy, input_handler);
  record_debug(recording);
  record_debug(copy);

  write_offset = (RecordOffset_t){0};
  readOffset = (RecordOffset_t){0};
  while (record_playback(copy, input_handler, &readOffset)) {
  }

  record_debug(recording);
  record_debug(copy);

  RecordOffset_t copyRead = {0};
  while (record_playback(copy, input_handler, &copyRead)) {
  }

  record_debug(recording);
  record_debug(copy);

  record_free(copy);
  record_free(recording);
}
