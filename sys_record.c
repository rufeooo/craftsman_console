
#include <string.h>

#include "macro.h"
#include "sys_record.h"

static char *restrict buf;
static int allocBuf;
static int usedBuf;
static bool playback;

bool
sys_recordInit()
{
  usedBuf = 0;
  allocBuf = 4 * 1024;
  free(buf);
  buf = malloc(allocBuf);

  return buf != 0;
}

static bool
sys_recordRealloc(int bytesNeeded)
{
  if (bytesNeeded <= 0)
    return true;

  char *const newBuf = realloc(buf, allocBuf * 2);
  if (!newBuf)
    return false;

  buf = newBuf;
  allocBuf *= 2;

  return true;
}

bool
sys_recordAppend(size_t len, const char *input)
{
  if (playback)
    return false;
  if (!sys_recordRealloc((len + usedBuf + 1) - allocBuf))
    return false;

  memcpy(buf + usedBuf, input, len);
  usedBuf += len;
  buf[usedBuf++] = 0;

  return true;
}

void
sys_recordPlayback(sys_recordEvent handler)
{
  playback = true;
  size_t length = { 0 };
  for (int i = 0; i < usedBuf; i += (length + 1)) {
    length = strlen(buf + i);
    handler(length, buf + i);
  }
  playback = false;
}

void
sys_recordShutdown()
{
  usedBuf = 0;
  allocBuf = 0;
  FREE(buf);
}
