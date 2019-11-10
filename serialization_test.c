#include <stdio.h>

#include "record.c"

RecordRW_t frame_store;

int
main()
{
  uint64_t bytes = 0;

  frame_store.rec = record_alloc();
  for (int i = 0; i < 10; ++i) {
    bytes = i;
    record_append(frame_store.rec, sizeof(bytes), (void *) &bytes,
                  &frame_store.write);
  }

  printf("%d written, record len %zu\n", frame_store.write.byte_count,
         record_length(frame_store.rec));

  for (int i = 0; i < 11; ++i) {
    bool r = record_read_bytes(frame_store.rec, sizeof(bytes),
                               (void *) &bytes, &frame_store.read);
    printf("r %d: %zx\n", r, bytes);
  }

  record_free(frame_store.rec);

  return 0;
}
