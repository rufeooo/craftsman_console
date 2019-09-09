
#pragma once

typedef void (*sys_inputEvent)(size_t strlen, char *str);

bool sys_inputInit();
bool sys_inputPoll(sys_inputEvent handler);
bool sys_inputShutdown();