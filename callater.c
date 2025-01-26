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

#define CALLATER_M256_AT(vec, idx) vec.m256_f32[idx]

#else

#define CALLATER_M256_AT(vec, idx) vec[idx]

#endif

#define CALLATER_FLT_AS_INT(f) \
((union{float asFloat; int32_t asInt;}){.asFloat = f}.asInt)

typedef struct CallaterTable
{
    size_t cap;
    size_t count;
    size_t noopCount;
    void(**funcs)(void*);
    void **args;
    uint64_t startSec;
    float *invokeTimes;
    float *originalDelays; // if neg, then no repeat
    float minInvokeTime;
    unsigned char delaysPtrOffset;
} CallaterTable;

static CallaterTable table = {0};

float CallaterCurrentTime()
{
    #ifdef _WIN32
    uint64_t tickCount = GetTickCount64();
    uint64_t secs = tickCount / 1000;
    uint64_t rem = tickCount % 1000;
    return (secs - table.startSec) + (rem / 1000.0f);
    #else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);
    
    return ts.tv_sec - table.startSec + ts.tv_nsec / 1000000000.0f;
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
        memmove(aligned, (char*)ret + *offset, szmin(size, oldSize));
    
    *offset = newOffset;
    return aligned;
}

void CallaterInit()
{
    table = (CallaterTable){0};
    
    table.startSec = CallaterCurrentTime();
    table.count  = 0;
    table.cap    = 64;
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
    __m256 infVec = _mm256_set1_ps(INFINITY);
    size_t i;
    for(i = 0 ; i + 7 < table.count ; i += 8)
    {
        __m256 curTimeVec = _mm256_load_ps(table.invokeTimes + i);
        __m256 results = _mm256_cmp_ps(curTimeVec, infVec, _CMP_EQ_OQ);

        for(int j = 0 ; j < 8 ; j++)
        {
            if(CALLATER_FLT_AS_INT( CALLATER_M256_AT(results, j) ) == -1)
            {
                CallaterPopFunc(i + j);
            }
        }
    }
    
    int remaining = table.count - i;
    for(int j = 0 ; j < remaining ; j++)
    {
        if(CALLATER_FLT_AS_INT(table.invokeTimes[i + j]) == -1)
        {
            CallaterPopFunc(i + j);
        }
    }
    
    table.noopCount = 0;
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

static void CallaterTick(float curTime)
{
    const __m256 curTimeVec = _mm256_set1_ps(curTime);
    
    size_t i;
    for(i = 0 ; i + 7 < table.count ; i += 8)
    {
        __m256 tableDelaysVec = _mm256_load_ps(table.invokeTimes + i);
        __m256 results = _mm256_cmp_ps(curTimeVec, tableDelaysVec, _CMP_GE_OQ);
        
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
    
    CallaterTick(curTime);
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
