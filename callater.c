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
    void** args;
    float* delays;
    float lastUpdated;
    float noUpdateTimeAccum;
    float minDelay;
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

static void *CallaterAlignedAlloc(size_t size, unsigned char alignment, unsigned char *offset)
{
    void *ret = calloc(size + (alignment - 1), 1);
    void *aligned = (void*)(((uintptr_t)ret + (alignment - 1)) & ~(alignment - 1));
    *offset = (char*)aligned - (char*)ret;
    return aligned;
}

static size_t szmin(size_t a, size_t b)
{
    return a < b ? a : b;
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
    table.lastUpdated = CallaterCurrentTime();
    
    table.count = 0;
    table.cap   = 64;
    table.funcs  = calloc(table.cap, sizeof(*table.funcs));
    table.args   = calloc(table.cap, sizeof(*table.args));
    table.delays = CallaterAlignedAlloc(table.cap * sizeof(*table.delays), 32, &table.delaysPtrOffset);
    table.minDelay = INFINITY;
}

static void CallaterNoop(void* arg)
{
    (void)arg;
}

static void CallaterPopFunc(size_t idx)
{
    table.funcs[idx]  = table.funcs[table.count - 1];
    table.delays[idx] = table.delays[table.count - 1];
    table.args[idx]   = table.args[table.count - 1];
    
    table.count -= 1;
}

static void CallaterCleanupTable()
{
    for(size_t i = 0 ; i < table.count ; i++)
    {
        if (table.funcs[i] == CallaterNoop)
            CallaterPopFunc(i);
    }
}

static void CallaterMaybeGrowTable()
{
    if (table.cap <= table.count)
    {
        const size_t newCap = table.cap * 2;
        table.funcs  = realloc(table.funcs,  newCap * sizeof(*table.funcs));
        table.args   = realloc(table.args,   newCap * sizeof(*table.args));
        table.delays = CallaterAlignedRealloc(table.delays, newCap * sizeof(*table.delays), table.cap * sizeof(*table.delays), 32, &table.delaysPtrOffset);
        table.cap = newCap;
    }
}

static void CallaterCallFunc(size_t idx)
{
    table.funcs[idx](table.args[idx]);
    table.funcs[idx] = CallaterNoop;
    table.delays[idx] = INFINITY;
    table.noopCount++;
}

static void UpdateDelayAccumulated()
{
    const float currentTime = CallaterCurrentTime();
    const float deltaTime = currentTime - table.lastUpdated;
    table.lastUpdated = currentTime;
    table.noUpdateTimeAccum += deltaTime;
}

static void CallaterForceUpdate()
{
    const __m256 zero = _mm256_setzero_ps();
    const __m256 deltaVec = _mm256_set1_ps(table.noUpdateTimeAccum);
    
    size_t i;
    for(i = 0 ; i + 7 < table.count ; i += 8)
    {
        float *curVec = (table.delays + i);
        __m256 tableDelaysVec = _mm256_load_ps(curVec);
        __m256 tableDelaysSubtracted = _mm256_sub_ps(tableDelaysVec, deltaVec);
        _mm256_store_ps(curVec, tableDelaysSubtracted);
        
        __m256 results = _mm256_cmp_ps(tableDelaysSubtracted, zero, _CMP_LE_OS);
        const int(*asInts)[8] = (void*)&results;
        for(int j = 0 ; j < 8 ; j++)
        {
            if ((*asInts)[j])
            {
                CallaterCallFunc(i + j);
            }
        }
    }
    
    const size_t remaining = table.count - i;
    
    for(int j = 0 ; j < remaining ; j++)
    {
        table.delays[j + i] -= table.noUpdateTimeAccum;
        if (table.delays[j + i] <= 0)
        {
            CallaterCallFunc(j + i);
        }
    }
    
    table.minDelay -= table.noUpdateTimeAccum;
    if(table.minDelay < 0)
    {
        float newMinDelay = INFINITY;
        for(size_t i = 0 ; i < table.count ; i++)
        {
            if(table.delays[i] < newMinDelay)
            {
                newMinDelay = table.delays[i];
            }
        }
        table.minDelay = newMinDelay;
    }
    
    table.noUpdateTimeAccum = 0;
}

void CallaterUpdate()
{
    UpdateDelayAccumulated();
    
    if(table.noUpdateTimeAccum < table.minDelay)
    {
        // no function would be called anyway, wait until the delay accumulated is big enough
        return;
    }
    
    CallaterForceUpdate();
}

void CallaterInvoke(void(*func)(void*), void* arg, float delay)
{
    CallaterMaybeGrowTable();
    table.funcs[table.count] = func;
    table.delays[table.count] = delay;
    table.args[table.count] = arg;
    
    UpdateDelayAccumulated();
    
    CallaterForceUpdate();
    
    if(delay < table.minDelay)
    {
        table.minDelay = delay;
    }
    
    table.count += 1;
    
    if (table.noopCount >= 8)
        CallaterCleanupTable();
}

void CallaterInvokeNull(void(*func)(void*), float delay)
{
    CallaterInvoke(func, NULL, delay);
}

