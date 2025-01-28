#include "callater.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
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
    uint64_t cap;
    uint64_t count;
    uint64_t noopCount;
    uint64_t nextEmptySpot;
    void(**funcs)(void*, CallaterRef);
    void **args;
    uint64_t startSec;
    uint64_t clockFreq;
    uint64_t *groupIDs;
    float *invokeTimes;
    float *repeatRates; // if neg, then no repeat
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

static uint64_t szmin(uint64_t a, uint64_t b)
{
    return a < b ? a : b;
}

static void *CallaterAlignedAlloc(uint64_t size, unsigned char alignment, unsigned char *offset)
{
    void *ret = calloc(size + (alignment - 1), 1);
    void *aligned = (void*)(((uintptr_t)ret + (alignment - 1)) & ~(alignment - 1));
    *offset = (char*)aligned - (char*)ret;
    return aligned;
}

static void *CallaterAlignedRealloc(void *ptr, uint64_t size, uint64_t oldSize, unsigned char alignment, unsigned char *offset)
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
    table.repeatRates = malloc(table.cap * sizeof(*table.repeatRates));
    table.groupIDs = calloc(table.cap, sizeof(*table.groupIDs));
    table.nextEmptySpot = 0;
    
    table.minInvokeTime = INFINITY;
}

static void CallaterNoop(void *arg, CallaterRef ref)
{
    (void)arg;
    (void)ref;
}

static void CallaterPopFunc(uint64_t idx)
{
    // TODO emptySpots
    // table.funcs[idx] = table.funcs[table.count - 1];
    // table.invokeTimes[idx] = table.invokeTimes[table.count - 1];
    // table.args[idx]  = table.args[table.count - 1];
    // table.repeatRates[idx] = table.repeatRates[table.count - 1];
    // table.groupIDs[idx] = table.groupIDs[table.count - 1];
    // table.count -= 1;
    
    table.funcs[idx] = CallaterNoop;
    table.args[idx] = NULL;
    table.invokeTimes[idx] = INFINITY;
    table.groupIDs[idx] = 0;
    table.repeatRates[idx] = INFINITY;
    table.noopCount++;
}

// static void CallaterCleanupTable()
// {
//     __m256 infVec = _mm256_set1_ps(INFINITY);
//     uint64_t i;
//     for(i = 0 ; i + 7 < table.count ; i += 8)
//     {
//         __m256 curTimeVec = _mm256_load_ps(table.invokeTimes + i);
//         __m256 results = _mm256_cmp_ps(curTimeVec, infVec, _CMP_EQ_OQ);
// 
//         for(int j = 0 ; j < 8 ; j++)
//         {
//             if(CALLATER_FLT_AS_INT( CALLATER_M256_AT(results, j) ) == -1)
//             {
//                 CallaterPopFunc(i + j);
//             }
//         }
//     }
//     
//     int remaining = table.count - i;
//     for(int j = 0 ; j < remaining ; j++)
//     {
//         if(CALLATER_FLT_AS_INT(table.invokeTimes[i + j]) == -1)
//         {
//             CallaterPopFunc(i + j);
//         }
//     }
//     
//     table.noopCount = 0;
// }

static void CallaterReallocTable(uint64_t newCap)
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
    table.repeatRates = realloc(table.repeatRates, newCap * sizeof(*table.repeatRates));
    table.groupIDs = realloc(table.groupIDs, newCap * sizeof(*table.groupIDs));
    table.cap = newCap;
}

static bool CallaterMaybeGrowTable()
{
    if(table.cap <= table.count)
    {
        const uint64_t newCap = table.cap * 2;
        CallaterReallocTable(newCap);
        return true;
    }
    return false;
}

static void CallaterCallFunc(uint64_t idx, float curTime)
{
    table.funcs[idx](table.args[idx], idx);
    if(table.repeatRates[idx] < 0)
    {
        // table.funcs[idx] = CallaterNoop;
        // table.invokeTimes[idx] = INFINITY;
        // table.groupIDs[idx] = 0;
        // table.noopCount++;
        CallaterPopFunc(idx);
    }
    else
    {
        table.invokeTimes[idx] = curTime + table.repeatRates[idx];
    }
}

static void CallaterTick(float curTime)
{
    const __m256 curTimeVec = _mm256_set1_ps(curTime);
    
    uint64_t i;
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
    for(uint64_t i = 0 ; i < table.count ; i++)
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
        // if(table.noopCount > table.count / 2)
        //     CallaterCleanupTable();
        return;
    }
    
    CallaterTick(curTime);
}

CallaterRef CallaterInvoke(void(*func)(void*, CallaterRef), void* arg, float delay)
{
    return CallaterInvokeID(func, arg, delay, 0);
}

CallaterRef CallaterInvokeID(void(*func)(void*, CallaterRef), void *arg, float delay, uint64_t groupId)
{
    if(table.nextEmptySpot == (uint64_t)-1)
    {
        CallaterMaybeGrowTable();
        table.nextEmptySpot = table.count;
        table.count += 1;
    }
    else if(table.nextEmptySpot != table.count)
    {
        // next empty spot is in the middle of the array
        // aka we're taking the spot of a noop and not increasing size
        table.noopCount -= 1;
    }
    else
    {
        // next empty spot is at the back
        table.count += 1;
    }
    
    float curTime = CallaterCurrentTime();
    
    uint64_t nextSpot = table.nextEmptySpot;
    table.funcs      [nextSpot] = func;
    table.invokeTimes[nextSpot] = delay + curTime;
    table.args       [nextSpot] = arg;
    table.repeatRates[nextSpot] = -1;
    table.groupIDs   [nextSpot] = groupId;
    
    if(table.invokeTimes[nextSpot] < table.minInvokeTime)
    {
        table.minInvokeTime = table.invokeTimes[nextSpot];
    }
    
    // assigning a new table.nextEmptySpot
    if(table.noopCount == 0)
    {
        if(table.count >= table.cap)
        {
            table.nextEmptySpot = (uint64_t)-1;
        }
        else
        {
            table.nextEmptySpot = table.count;
        }
    }
    else
    {
        uint64_t nextEmptySpot = (uint64_t)-1;
        for(uint64_t i = 0 ; i < table.count ; i++)
        {
            if(table.funcs[i] == CallaterNoop)
            {
                nextEmptySpot = i;
                break;
            }
        }
        table.nextEmptySpot = nextEmptySpot;
    }
    
    return nextSpot;
}

CallaterRef CallaterInvokeRepeat(void(*func)(void*, CallaterRef), void *arg, float firstDelay, float repeatRate)
{
    return CallaterInvokeRepeatID(func, arg, firstDelay, repeatRate, 0);
}

CallaterRef CallaterInvokeRepeatID(void(*func)(void*, CallaterRef), void *arg, float firstDelay, float repeatRate, uint64_t groupId)
{
    CallaterRef ret = CallaterInvokeID(func, arg, firstDelay, groupId);
    CallaterSetRepeatRate(ret, repeatRate);
    return ret;
}

void CallaterCancelGroup(uint64_t groupId)
{
    for(uint64_t i = 0 ; i < table.count ; i++)
    {
        if(table.groupIDs[i] == groupId)
        {
            CallaterPopFunc(i);
            // i--;
        }
    }
}

void CallaterCancelFunc(void(*func)(void*, CallaterRef))
{
    for(uint64_t i = 0 ; i < table.count ; i++)
    {
        if(table.funcs[i] == func)
        {
            CallaterPopFunc(i);
            // i--;
        }
    }
}

CallaterRef CallaterFuncRef(void(*func)(void*, CallaterRef))
{
    for(uint64_t i = 0 ; i < table.count ; i++)
    {
        if(table.funcs[i] == func)
        {
            return i;
        }
    }
    return CALLATER_ERR_REF;
}

void CallaterStopRepeat(CallaterRef ref)
{
    table.repeatRates[ref] = -1;
}

void CallaterSetRepeatRate(CallaterRef ref, float newRepeatRate)
{
    table.repeatRates[ref] = newRepeatRate;
}

void CallaterSetID(CallaterRef ref, uint64_t groupId)
{
    table.groupIDs[ref] = groupId;
}

float CallaterGetRepeatRate(CallaterRef ref)
{
    return table.repeatRates[ref];
}

uint64_t CallaterGetID(CallaterRef ref)
{
    return table.groupIDs[ref];
}

void CallaterShrinkToFit()
{
    //if(table.noopCount >= 8)
        //CallaterCleanupTable();
    
    CallaterReallocTable(table.count);
}

void CallaterDeinit()
{
    free(table.funcs);
    free(table.args);
    free(table.invokeTimes - table.delaysPtrOffset);
    free(table.repeatRates);
    table = (CallaterTable){0};
}
