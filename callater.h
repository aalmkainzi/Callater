#ifndef CALLATER_H
#define CALLATER_H

#ifndef CALLATER_NO_SHORT_NAME
    
    #define Invoke(func, arg, delay) \
    CallaterInvoke(func, arg, delay)
    
#endif

void CallaterInvoke(void(*func)(void*), void* arg, float delay);
void CallaterInvokeNull(void(*func)(void*), float delay);
void CallaterUpdate();
void CallaterInit();

#endif