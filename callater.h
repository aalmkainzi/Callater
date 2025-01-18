#ifndef CALLATER_H
#define CALLATER_H

#ifndef CALLATER_NO_SHORT_NAMES
    
    #define Invoke CallaterInvoke
    #define InvokeNull CallaterInvokeNull
    
#endif

void CallaterInvoke(void(*func)(void*), void* arg, float delay);
void CallaterInvokeNull(void(*func)(void*), float delay);
void CallaterUpdate();
void CallaterInit();

void CallaterShrinkToFit(); // realloc table to match table.count
void CallaterDeinit(); // free all buffers
#endif