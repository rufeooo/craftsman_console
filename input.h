
#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef void (*InputEvent_t)(size_t strlen, char *str);

bool input_init();
bool input_poll(InputEvent_t handler);
bool input_shutdown();
