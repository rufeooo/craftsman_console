
#include <stdio.h>

#include "macro.h"
#include "record.h"

static Record_t *recording;
static RecordOffset_t write_offset;

void
input_handler(size_t strlen, char *str)
{
  record_append(recording, strlen, str, &write_offset);

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

  record_append(recording, 0, 0, &write_offset);
  RecordOffset_t readOffset = { 0 };
  record_playback(recording, test_handler, &readOffset);

  record_append(recording, 1, "a", &write_offset);
  record_append(recording, 1, "b", &write_offset);
  record_append(recording, 1, "c", &write_offset);
  record_append(recording, 1, "d", &write_offset);

  record_debug(recording);
  Record_t *copy = record_clone(recording);
  record_debug(copy);
  if (record_compare(recording, copy)) {
    puts("clone not equal!");
  }
  write_offset = (RecordOffset_t){ 0 };

  record_playback_all(copy, input_handler);
  record_debug(recording);
  record_debug(copy);

  write_offset = (RecordOffset_t){ 0 };
  readOffset = (RecordOffset_t){ 0 };
  while (record_playback(copy, input_handler, &readOffset)) {
  }

  record_debug(recording);
  record_debug(copy);

  RecordOffset_t copyRead = { 0 };
  while (record_playback(copy, input_handler, &copyRead)) {
  }

  record_debug(recording);
  record_debug(copy);

  record_free(copy);
  record_free(recording);
}
