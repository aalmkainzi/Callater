#ifndef CALLATER_H
#define CALLATER_H

#include <stdint.h>

#ifndef CALLATER_NO_SHORT_NAMES
    
    #define Invoke         CallaterInvoke
    #define InvokeRepeat   CallaterInvokeRepeat
    #define InvokeID       CallaterInvokeID
    #define InvokeRepeatID CallaterInvokeRepeatID
    
#endif

#define CALLATER_ERR_REF ((CallaterRef)-1)

typedef uint64_t CallaterRef;

// initialize the Callater context
void CallaterInit();

// adds the function `func` to be called after `delay` time, with `arg` passed to it
CallaterRef CallaterInvoke(void(*func)(void*, CallaterRef), void *arg, float delay);

// same as `CallaterInvoke`, except you can use `groupId` as a handle to the invocations (e.g. when using `CallaterCancelGroup(uint64_t groupId)`)
CallaterRef CallaterInvokeID(void(*func)(void*, CallaterRef), void *arg, float delay, uint64_t groupId);

// calls `func` after `firstDelay` seconds, then every `repeatRate` seconds
CallaterRef CallaterInvokeRepeat(void(*func)(void*, CallaterRef), void *arg, float firstDelay, float repeatRate);

CallaterRef CallaterInvokeRepeatID(void(*func)(void*, CallaterRef), void *arg, float firstDelay, float repeatRate, uint64_t groupId);

// this must be called for the functions added with `CallaterInvoke` to actually get invoked
// basically you should call this once every frame
void CallaterUpdate();

// gets the temporary ref of a function that was added
// if multiple occurances of `func` exist, it doesn't necessarily get the next one to be invoked, nor the most newly inserted
CallaterRef CallaterFuncRef(void(*func)(void*, CallaterRef));

// remove all occurances of `func` from being invoked
void CallaterCancelFunc(void(*func)(void*, CallaterRef));

// remove all invocations associated with `groupId`
void CallaterCancelGroup(uint64_t groupId);

// stops the referenced invocation from repeating
void CallaterStopRepeat(CallaterRef ref);

// changes the function to be invoked
void CallaterSetFunc(CallaterRef ref, void(*func)(void*, CallaterRef));

// changes the repeat rate of an invocation. Can also be used to make non-repeating invocation be repeating
void CallaterSetRepeatRate(CallaterRef ref, float newRepeatRate);

// changes the groupId of the referenced invocation
void CallaterSetID(CallaterRef ref, uint64_t groupId);

float CallaterGetRepeatRate(CallaterRef ref);

uint64_t CallaterGetID(CallaterRef ref);

// realloc to match size
void CallaterShrinkToFit();

// frees the memory
void CallaterDeinit();

#endif
