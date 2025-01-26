#ifndef CALLATER_H
#define CALLATER_H

#include <stdint.h>

#ifndef CALLATER_NO_SHORT_NAMES
    
    #define Invoke CallaterInvoke
    #define InvokeRepeat CallaterInvokeRepeat
    #define InvokeID CallaterInvokeID
    #define InvokeRepeatID CallaterInvokeRepeatID
    
#endif

// initialize the Callater context
void CallaterInit();

// adds the function `func` to be called after `delay` time, with `arg` passed to it
void CallaterInvoke(void(*func)(void*), void* arg, float delay);

// same as `CallaterInvoke`, except you can use `id` as a handle to the invocation (e.g. when using `CallaterCancelAllID(uint64_t id)`)
void CallaterInvokeID(void(*func)(void*), void *arg, float delay, uint64_t id);

// calls `func` after `firstDelay` seconds, then every `repeatRate` seconds
void CallaterInvokeRepeat(void(*func)(void*), void *arg, float firstDelay, float repeatRate);

void CallaterInvokeRepeatID(void(*func)(void*), void *arg, float firstDelay, float repeatRate, uint64_t id);

// this must be called for the functions added with `CallaterInvoke` to actually get invoked
// basically you should call this once every frame
void CallaterUpdate();

// remove an occurance of `func` from being invoked
void CallaterCancel(void(*func)(void*));

// remove all occurances of `func` from being invoked
void CallaterCancelAll(void(*func)(void*));

// remove all invocations associated with `id`
void CallaterCancelAllID(uint64_t id);

// realloc to match size
void CallaterShrinkToFit();

// frees the memory
void CallaterDeinit();

#endif