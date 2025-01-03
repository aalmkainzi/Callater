#ifndef CALLATER_H
#define CALLATER_H

#define EnsureOne(a) a
#define Invoke(func, timeOrArg, ...) \
CallaterInvokeLater( \
    func, \
    (__VA_OPT__((void)) NULL __VA_OPT__(, timeOrArg)), \
    (__VA_OPT__((void)) timeOrArg __VA_OPT__(, EnsureOne(__VA_ARGS__))) \
)

void CallaterInvokeLater(void(*func)(void*), void* arg, float delay);
void CallaterUpdate();
void CallaterInit();

#endif