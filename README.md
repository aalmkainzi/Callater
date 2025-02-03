# Callater: Delayed Function Calls in C

A small C library for invoking functions after a specified delay without blocking the execution flow.

It uses AVX SIMD instructions to efficently find and call functions whose delay is expired.

### Usage

Quick examples:

```C
#include "callater.h"
#include <stdio.h>
#include <stdlib.h>

void exitMsg(void *msg, CallaterRef invokeRef)
{
    printf("%s\n", (char*)msg);
    
    CallaterDeinit();
    exit(0);
}

void printNum(void *num, CallaterRef invokeRef)
{
    printf("%d\n", *(int*)num);
}

int main()
{
    CallaterInit();
    
    int nums[100];
    for(int i = 0 ; i < 10 ; i++)
    {
        nums[i] = i;
        Invoke(printNum, &nums[i], i / 2.0f);
    }
    
    Invoke(exitMsg, "bye", 5.0f);
    
    while(1)
        CallaterUpdate();
}
```

```C
#include "callater.h"
#include <stdio.h>

void printMsg(void *msg, CallaterRef invokeRef)
{
    printf("%s\n", (char*)msg);
}

int main()
{
    CallaterInit();
    
    InvokeRepeat(printMsg, "marco", 0, 1);
    InvokeRepeat(printMsg, "polo\n", 0.5f, 1);
    
    while(1)
        CallaterUpdate();
    
    CallaterDeinit();
}
```

API Reference

```C
// initialize the Callater context
void CallaterInit();

// Adds the function `func` to be called after `delay` time, with `arg` passed
// Returns the reference to the invocation
CallaterRef CallaterInvoke(void(*func)(void*, CallaterRef), void *arg, float delay);

// Same as `CallaterInvoke`, except you can use `groupId` as a handle to the invocations
// (e.g. when using `CallaterCancelGID(uint64_t groupId)`)
// Returns the reference to the invocation
// NOTE groupId -1 is reserved
CallaterRef CallaterInvokeGID(void(*func)(void*, CallaterRef), void *arg, float delay, uint64_t groupId);

// Calls `func` after `firstDelay` seconds, then every `repeatRate` seconds
// Returns the reference to the invocation
CallaterRef CallaterInvokeRepeat(void(*func)(void*, CallaterRef), void *arg, float firstDelay, float repeatRate);

CallaterRef CallaterInvokeRepeatGID(void(*func)(void*, CallaterRef), void *arg, float firstDelay, float repeatRate, uint64_t groupId);

// This must be called for the invocations added to actually get called
// basically you should call this once every frame
void CallaterUpdate();

// Returns the seconds from now to when the invocation will happen
float CallaterInvokesAfter(CallaterRef ref);

// Gets the invocation reference of a function that was added
// if multiple occurances of `func` exist, it doesn't necessarily get the next one to be invoked, nor the most newly inserted
CallaterRef CallaterFuncRef(void(*func)(void*, CallaterRef));

// Remove all occurances of `func` from being invoked
void CallaterCancelFunc(void(*func)(void*, CallaterRef));

// Remove all invocations associated with `groupId`
void CallaterCancelGID(uint64_t groupId);

// Removes the referenced invocation
void CallaterCancel(CallaterRef ref);

// Stops the referenced invocation from repeating
void CallaterStopRepeat(CallaterRef ref);

// Changes the repeat rate of an invocation. Can also be used to make non-repeating invocation be repeating
// NOTE the new repeat rate will only take effect after the current invocation is done
void CallaterSetRepeatRate(CallaterRef ref, float newRepeatRate);

float CallaterGetRepeatRate(CallaterRef ref);

// Changes the function to be invoked
void CallaterSetFunc(CallaterRef ref, void(*func)(void*, CallaterRef));

typeof(void(*)(void*, CallaterRef)) CallaterGetFunc(CallaterRef ref);

// Changes the arg to be passed when the function is invoked
void CallaterSetArg(CallaterRef ref, void *arg);

void *CallaterGetArg(CallaterRef ref);

// Changes the groupId of the referenced invocation
void CallaterSetGID(CallaterRef ref, uint64_t groupId);

uint64_t CallaterGetGID(CallaterRef ref);

// Returns the number of invocations associated with `groupId`
uint64_t CallaterGroupCount(uint64_t groupId);

// Fills the array `refsOut` with the invocation references associated with `groupId`
// Returns the number of references that were added to the pointer
uint64_t CallaterGetGroupRefs(CallaterRef *refsOut, uint64_t groupId);

// Shrinks the context to match size, in case you want lower memory usage
void CallaterShrinkToFit();

// Releases all memory
void CallaterDeinit();
```
