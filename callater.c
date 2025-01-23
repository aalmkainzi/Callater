#include "callater.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <immintrin.h>

#ifdef _WIN32
#include <windows.h>
#include <sysinfoapi.h>
#endif

typedef struct CallaterTable
{
    size_t count;
    size_t cap;
    size_t nextFuncIdx;
    void(**funcs)(void*);
    void **args;
    float *invokeTimes;
    float startTime;
    unsigned char delaysPtrOffset;
} CallaterTable;

static CallaterTable table;

float CallaterCurrentTime()
{
    #ifdef _WIN32
    return GetTickCount64() / 1000.0f;
    #else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    
    return ts.tv_sec + ts.tv_nsec / 1000000000.0f;
    #endif
}

static size_t szmin(size_t a, size_t b)
{
    return a < b ? a : b;
}

static void *CallaterAlignedAlloc(size_t size, unsigned char alignment, unsigned char *offset)
{
    void *ret = calloc(size + (alignment - 1), 1);
    void *aligned = (void*)(((uintptr_t)ret + (alignment - 1)) & ~(alignment - 1));
    *offset = (char*)aligned - (char*)ret;
    return aligned;
}

static void *CallaterAlignedRealloc(void *ptr, size_t size, size_t oldSize, unsigned char alignment, unsigned char *offset)
{
    void *ret = realloc((unsigned char*)ptr - *offset, size + (alignment - 1));
    void *aligned = (void*)(((uintptr_t)ret + (alignment - 1)) & ~(alignment - 1));
    unsigned char newOffset = (char*)aligned - (char*)ret;
    
    if(newOffset != *offset)
        memmove(aligned, ret + *offset, szmin(size, oldSize));
    
    *offset = newOffset;
    return aligned;
}

void CallaterInit()
{
    table = (CallaterTable){0};
    table.startTime = CallaterCurrentTime();
    
    table.count = 0;
    table.cap   = 64;
    table.funcs  = calloc(table.cap, sizeof(*table.funcs));
    table.args   = calloc(table.cap, sizeof(*table.args));
    table.invokeTimes = CallaterAlignedAlloc(table.cap * sizeof(*table.invokeTimes), 32, &table.delaysPtrOffset);
    table.nextFuncIdx = (size_t)-1;
}

static void CallaterNoop(void* arg)
{
    (void)arg;
}

static void CallaterPopFuncs(size_t nb)
{
    table.count -= nb;
}

static void CallaterReallocTable(size_t newCap)
{
    table.funcs  = realloc(table.funcs,  newCap * sizeof(*table.funcs));
    table.args   = realloc(table.args,   newCap * sizeof(*table.args));
    table.invokeTimes =
    CallaterAlignedRealloc(
        table.invokeTimes,
        newCap    * sizeof(*table.invokeTimes),
        table.cap * sizeof(*table.invokeTimes),
        32,
        &table.delaysPtrOffset
    );
    table.cap = newCap;
}

static void CallaterMaybeGrowTable()
{
    if(table.cap <= table.count)
    {
        const size_t newCap = table.cap * 2;
        CallaterReallocTable(newCap);
    }
}

void CallaterUpdate()
{
    
}

static void CallaterInsertFunc(void(*func)(void*), void *arg, float invokeTime)
{
    
}

void CallaterInvoke(void(*func)(void*), void* arg, float delay)
{
    CallaterMaybeGrowTable();
    
    float invokeTime = delay + CallaterCurrentTime();
    
    CallaterInsertFunc(func, arg, invokeTime);
}

void CallaterInvokeNull(void(*func)(void*), float delay)
{
    CallaterInvoke(func, NULL, delay);
}

void CallaterInvokeRepeat(void(*func)(void*), void *arg, float firstDelay, float repeatRate)
{
    CallaterInvoke(func, arg, firstDelay);
    table.originalDelays[table.count - 1] = repeatRate;
}

void CallaterShrinkToFit()
{
    if(table.noopCount >= 8)
        CallaterCleanupTable();
    
    CallaterReallocTable(table.count);
}

void CallaterDeinit()
{
    free(table.funcs);
    free(table.args);
    free(table.delays - table.delaysPtrOffset);
    free(table.originalDelays);
    table = (CallaterTable){0};
}
