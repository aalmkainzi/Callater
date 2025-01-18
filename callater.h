#ifndef CALLATER_H
#define CALLATER_H

#ifndef CALLATER_NO_SHORT_NAMES
    
    #define Invoke CallaterInvoke
    #define InvokeNull CallaterInvokeNull
    
#endif

// initialize the Callater context
void CallaterInit();

// adds the function `func` to be called after `delay` time, with `arg` passed to it
void CallaterInvoke(void(*func)(void*), void* arg, float delay);

// same as above except NULL will be passed to `func`
void CallaterInvokeNull(void(*func)(void*), float delay);

// calls `func` after `firstDelay` seconds, then every `repeatDelay` seconds
void CallaterInvokeRepeat(void(*func)(void*), void *arg, float firstDelay, float repeatRate);

// this must be called for the functions added with `CallaterInvoke` to actually get called
// basically you should call this once every frame
void CallaterUpdate();

// realloc to match size
void CallaterShrinkToFit();

// free all buffers
void CallaterDeinit();

#endif