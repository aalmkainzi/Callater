#ifndef CALLATER_H
#define CALLATER_H

#define Cat(a, b) a##b

#define IfNoVargs(then, ...) \
Cat(IfNoVargs, 0 ## __VA_OPT__(1))(then)

#define IfNoVargs0(then) then

#define IfNoVargs01(then) 

#define Invoke(func, timeOrArg, ...) \
__VA_OPT__( \
    InvokeLaterArg(func, timeOrArg, (__VA_ARGS__)) \
) \
IfNoVargs(InvokeLater(func, timeOrArg), __VA_ARGS__)

void InvokeLaterArg(void(*func)(void*), void *arg, float delay);
void InvokeLater(void(*func)(), float delay);
void CallaterUpdate();
void CallaterInit();

#endif