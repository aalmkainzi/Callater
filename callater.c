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

#ifdef _MSC_VER

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
    uint64_t clockFreq;
    uint64_t *ownerIDs;
    float *invokeTimes;
    float *originalDelays; // if neg, then no repeat
    float minInvokeTime;
    unsigned char delaysPtrOffset;
} CallaterTable;

static CallaterTable table = {0};

struct timespec CallaterGetTimespec()
{
#if defined(__MINGW32__)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts;
#elif defined(_WIN32)
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return ts;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);
    return ts;
#endif
}

float CallaterCurrentTime()
{
#ifdef _WIN32
    uint64_t time;
    QueryPerformanceCounter((void*)&time);
    return (time - table.startSec * table.clockFreq) / (float) table.clockFreq;
#else
    struct timespec ts = CallaterGetTimespec();
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
#ifdef _WIN32
        QueryPerformanceFrequency((void*) &table.clockFreq);
#endif
    table.startSec = CallaterCurrentTime();
    table.count  = 0;
    table.cap    = 64;
    table.funcs  = calloc(table.cap, sizeof(*table.funcs));
    table.args   = calloc(table.cap, sizeof(*table.args));
    table.invokeTimes = CallaterAlignedAlloc(table.cap * sizeof(*table.invokeTimes), 32, &table.delaysPtrOffset);
    table.originalDelays = malloc(table.cap * sizeof(*table.originalDelays));
    table.ownerIDs = calloc(table.cap, sizeof(*table.ownerIDs));
    
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
    table.ownerIDs[idx] = table.ownerIDs[table.count - 1];
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
    table.ownerIDs = realloc(table.ownerIDs, newCap * sizeof(*table.ownerIDs));
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
        table.ownerIDs[idx] = 0;
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
    CallaterInvokeID(func, arg, delay, 0);
}

void CallaterInvokeNull(void(*func)(void*), float delay)
{
    CallaterInvoke(func, NULL, delay);
}

void CallaterInvokeID(void(*func)(void*), void *arg, float delay, uint64_t id)
{
    CallaterMaybeGrowTable();
    
    float curTime = CallaterCurrentTime();
    
    table.funcs[table.count] = func;
    table.invokeTimes[table.count] = delay + curTime;
    table.args[table.count] = arg;
    table.originalDelays[table.count] = -1;
    table.ownerIDs[table.count] = id;
    table.count += 1;
    
    if(table.invokeTimes[table.count] < table.minInvokeTime)
    {
        table.minInvokeTime = table.invokeTimes[table.count];
    }
}

void CallaterInvokeRepeat(void(*func)(void*), void *arg, float firstDelay, float repeatRate)
{
    CallaterInvokeRepeatID(func, arg, firstDelay, repeatRate, 0);
}

void CallaterInvokeRepeatID(void(*func)(void*), void *arg, float firstDelay, float repeatRate, uint64_t id)
{
    CallaterInvokeID(func, arg, firstDelay, id);
    table.originalDelays[table.count - 1] = repeatRate;
}

void CallaterCancelAllID(uint64_t id)
{
    for(size_t i = 0 ; i < table.count ; i++)
    {
        if(table.ownerIDs[i] == id)
        {
            CallaterPopFunc(i);
            i--;
        }
    }
}

void CallaterCancelAll(void(*func)(void*))
{
    for(size_t i = 0 ; i < table.count ; i++)
    {
        if(table.funcs[i] == func)
        {
            CallaterPopFunc(i);
            i--;
        }
    }
}

void CallaterCancel(void(*func)(void*))
{
    for(size_t i = 0 ; i < table.count ; i++)
    {
        if(table.funcs[i] == func)
        {
            CallaterPopFunc(i);
            return;
        }
    }
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
