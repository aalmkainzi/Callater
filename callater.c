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
    size_t cap;
    size_t count;
    size_t noopCount;
    void(**funcs)(void*);
    void **args;
    float *invokeTimes;
    float *originalDelays; // if neg, then no repeat
    // float lastUpdated;
    // float noUpdateTimeAccum;
    float minInvokeTime;
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
    
    table.count = 0;
    table.cap   = 64;
    table.funcs  = calloc(table.cap, sizeof(*table.funcs));
    table.args   = calloc(table.cap, sizeof(*table.args));
    table.invokeTimes = CallaterAlignedAlloc(table.cap * sizeof(*table.invokeTimes), 32, &table.delaysPtrOffset);
    table.originalDelays = malloc(table.cap * sizeof(*table.originalDelays));
    
    table.minInvokeTime = INFINITY;
}

static void CallaterNoop(void* arg)
{
    (void)arg;
}

static void CallaterPopFunc(size_t idx)
{
    table.funcs[idx] = table.funcs[table.count - 1];
    table.invokeTimes[idx] = table.invokeTimes[table.count - 1];
    table.args[idx]  = table.args[table.count - 1];
    table.originalDelays[idx] = table.originalDelays[table.count - 1];
    table.count -= 1;
}

static void CallaterCleanupTable()
{
    for(size_t i = 0 ; i < table.count ; i++)
    {
        if(table.funcs[i] == CallaterNoop)
            CallaterPopFunc(i);
    }
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
    table.originalDelays = realloc(table.originalDelays, newCap * sizeof(*table.originalDelays));
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

static void CallaterCallFunc(size_t idx, float curTime)
{
    table.funcs[idx](table.args[idx]);
    if(table.originalDelays[idx] < 0)
    {
        table.funcs[idx] = CallaterNoop;
        table.invokeTimes[idx] = INFINITY;
        table.noopCount++;
    }
    else
    {
        table.invokeTimes[idx] = curTime + table.originalDelays[idx];
    }
}

static void CallaterForceUpdate(float curTime)
{
    const __m256 curTimeVec = _mm256_set1_ps(curTime);
    
    size_t i;
    for(i = 0 ; i + 7 < table.count ; i += 8)
    {
        float *curVec = (table.invokeTimes + i);
        __m256 tableDelaysVec = _mm256_load_ps(curVec);
        
        __m256 results = _mm256_cmp_ps(curTimeVec, tableDelaysVec, _CMP_GE_OS);
        const int(*asInts)[8] = (void*)&results;
        for(int j = 0 ; j < 8 ; j++)
        {
            if((*asInts)[j])
            {
                CallaterCallFunc(i + j, curTime);
            }
        }
    }
    
    const int remaining = table.count - i;
    
    for(int j = 0 ; j < remaining ; j++)
    {
        if(table.invokeTimes[j + i] <= curTime)
        {
            CallaterCallFunc(j + i, curTime);
        }
    }
    
    float newMinInvokeTime = INFINITY;
    for(size_t i = 0 ; i < table.count ; i++)
    {
        if(table.invokeTimes[i] < newMinInvokeTime)
        {
            newMinInvokeTime = table.invokeTimes[i];
        }
    }
    table.minInvokeTime = newMinInvokeTime;
}

void CallaterUpdate()
{
    float curTime = CallaterCurrentTime();
    if(curTime < table.minInvokeTime)
    {
        if(table.noopCount > table.count / 2)
            CallaterCleanupTable();
        return;
    }
    
    CallaterForceUpdate(curTime);
}

void CallaterInvoke(void(*func)(void*), void* arg, float delay)
{
    CallaterMaybeGrowTable();
    
    float curTime = CallaterCurrentTime();
    
    table.funcs[table.count] = func;
    table.invokeTimes[table.count] = delay + curTime;
    table.args[table.count] = arg;
    table.originalDelays[table.count] = -1;
    table.count += 1;
    
    if(table.invokeTimes[table.count] < table.minInvokeTime)
    {
        table.minInvokeTime = table.invokeTimes[table.count];
    }
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
    free(table.invokeTimes - table.delaysPtrOffset);
    free(table.originalDelays);
    table = (CallaterTable){0};
}
