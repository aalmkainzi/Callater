# Callater: Delayed Function Calls in C

A small C library for invoking functions after a specified delay without blocking the execution flow.

It uses AVX SIMD instructions to efficently find and call functions whose delay is expired.

### Usage

Quick examples:

```C
#include "callater.h"
#include <stdio.h>
#include <stdlib.h>

void exitMsg(void *msg)
{
    printf("%s\n", (char*)msg);
    
    CallaterDeinit();
    exit(0);
}

void printNum(void *num)
{
    printf("%d\n", *(int*)num);
}

int main()
{
    CallaterInit();
    
    int nums[1000];
    for(int i = 0 ; i < 1000 ; i++)
    {
        nums[i] = i;
        Invoke(printNum, &nums[i], i / 100.0f);
    }
    
    Invoke(exitMsg, "bye", 10.0f);
    
    while(1)
        CallaterUpdate();
}
```

```C
#include "callater.h"
#include <stdio.h>

void printMsg(void *msg)
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

// adds the function `func` to be called after `delay` time, with `arg` passed to it
void CallaterInvoke(void(*func)(void*), void* arg, float delay);

// calls `func` after `firstDelay` seconds, then every `repeatRate` seconds
void CallaterInvokeRepeat(void(*func)(void*), void *arg, float firstDelay, float repeatRate);

// this must be called for the functions added with `CallaterInvoke` to actually get invoked
// basically you should call this once every frame
void CallaterUpdate();

// realloc to match size
void CallaterShrinkToFit();

// frees the memory
void CallaterDeinit();
```
