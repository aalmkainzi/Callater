#ifndef CALLATER_H
#define CALLATER_H

#define Invoke(func, timeOrArg, ...) \
CallaterInvokeLater(func, (NULL __VA_OPT__(, timeOrArg)), (timeOrArg __VA_OPT__(, __VA_ARGS__)))

void CallaterInvokeLater(void(*func)(void*), void* arg, float delay);
void CallaterUpdate();
void CallaterInit();

#endif