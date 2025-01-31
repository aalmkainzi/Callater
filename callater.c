#include "callater.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
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
    uint64_t lastRealInvocation; // last elm that isn't Noop
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

static CallaterTable table = { 0 };

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
#ifdef CALLATER_TEST
    return mock_current_time;
#else
#ifdef _WIN32
    uint64_t time;
    QueryPerformanceCounter((void*)&time);
    return (time - table.startSec * table.clockFreq) / (float) table.clockFreq;
#else
    struct timespec ts = CallaterGetTimespec();
    return ts.tv_sec - table.startSec + ts.tv_nsec / 1000000000.0f;
#endif
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
    table.lastRealInvocation = -1;
    table.minInvokeTime = INFINITY;
}

static void CallaterNoop(void *arg, CallaterRef ref)
{
    (void)arg;
    (void)ref;
}

static void CallaterPopInvoke(uint64_t idx)
{
    table.noopCount += (table.funcs[idx] != CallaterNoop);
    table.funcs[idx] = CallaterNoop;
    table.args[idx] = NULL;
    table.invokeTimes[idx] = INFINITY;
    table.groupIDs[idx] = (uint64_t)-1;
    table.repeatRates[idx] = INFINITY;
}

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
    table.funcs[idx](table.args[idx], (CallaterRef){idx});
    if(table.repeatRates[idx] < 0)
    {
        CallaterPopInvoke(idx);
    }
    else
    {
        table.invokeTimes[idx] = curTime + table.repeatRates[idx];
    }
}

static void CallaterFindNewLastInvocation(uint64_t startFrom)
{
    uint64_t newLastRealInvocation = -1;
    for(uint64_t i = startFrom ; i != (uint64_t)-1 ; i--)
    {
        if(table.funcs[i] != CallaterNoop)
        {
            newLastRealInvocation = i;
            break;
        }
    }
    table.lastRealInvocation = newLastRealInvocation;
}

static void CallaterTick(float curTime)
{
    const __m256 curTimeVec = _mm256_set1_ps(curTime);
    
    uint64_t i;
    for(i = 0 ; i + 7 <= table.lastRealInvocation ; i += 8)
    {
        __m256 tableDelaysVec = _mm256_load_ps(table.invokeTimes + i);
        __m256 results = _mm256_cmp_ps(curTimeVec, tableDelaysVec, _CMP_GE_OQ);
        
        int mask = _mm256_movemask_ps(results);
        while (mask != 0)
        {
            unsigned long bit;
#ifdef _WIN32
            _BitScanForward(&bit, mask);
#else
            bit = __builtin_ctz(mask);
#endif
            mask &= ~(1 << bit);
            CallaterCallFunc(i + bit, curTime);
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
    
    if(table.funcs[table.lastRealInvocation] == CallaterNoop)
    {
        CallaterFindNewLastInvocation(table.lastRealInvocation - 1);
    }
}

void CallaterUpdate()
{
    float curTime = CallaterCurrentTime();
    
    // do we even need the second term? minInvokeTime should be enough
    if(curTime < table.minInvokeTime || table.lastRealInvocation == (uint64_t)-1)
    {
        return;
    }
    
    CallaterTick(curTime);
}

CallaterRef CallaterInvoke(void(*func)(void*, CallaterRef), void* arg, float delay)
{
    return CallaterInvokeGID(func, arg, delay, (uint64_t)-1);
}

CallaterRef CallaterInvokeGID(void(*func)(void*, CallaterRef), void *arg, float delay, uint64_t groupId)
{
    if(table.nextEmptySpot == (uint64_t)-1)
    {
        CallaterMaybeGrowTable();
        table.nextEmptySpot = table.count;
        table.lastRealInvocation = table.count;
        table.count += 1;
    }
    else if(table.nextEmptySpot != table.count)
    {
        // next empty spot is in the middle of the array
        // aka we're taking the spot of a noop and not increasing count
        table.noopCount -= 1;
    }
    else
    {
        // next empty spot is at the back
        table.lastRealInvocation = table.count;
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
        if(table.lastRealInvocation < table.count - 1)
        {
            table.nextEmptySpot = table.lastRealInvocation + 1;
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
    }
    return (CallaterRef){nextSpot};
}

CallaterRef CallaterInvokeRepeat(void(*func)(void*, CallaterRef), void *arg, float firstDelay, float repeatRate)
{
    return CallaterInvokeRepeatGID(func, arg, firstDelay, repeatRate, (uint64_t)-1);
}

CallaterRef CallaterInvokeRepeatGID(void(*func)(void*, CallaterRef), void *arg, float firstDelay, float repeatRate, uint64_t groupId)
{
    CallaterRef ret = CallaterInvokeGID(func, arg, firstDelay, groupId);
    CallaterSetRepeatRate(ret, repeatRate);
    return ret;
}

float CallaterInvokesAfter(CallaterRef ref)
{
    return CallaterCurrentTime() - table.invokeTimes[ref.ref];
}

void CallaterCancelGID(uint64_t groupId)
{
    for(uint64_t i = 0 ; i < table.count ; i++)
    {
        if(table.groupIDs[i] == groupId)
        {
            CallaterPopInvoke(i);
        }
    }
}

void CallaterCancelFunc(void(*func)(void*, CallaterRef))
{
    for(uint64_t i = 0 ; i < table.count ; i++)
    {
        if(table.funcs[i] == func)
        {
            CallaterPopInvoke(i);
        }
    }
}

CallaterRef CallaterFuncRef(void(*func)(void*, CallaterRef))
{
    for(uint64_t i = 0 ; i < table.count ; i++)
    {
        if(table.funcs[i] == func)
        {
            return (CallaterRef){i};
        }
    }
    return CALLATER_ERR_REF;
}

void CallaterCancel(CallaterRef ref)
{
    CallaterPopInvoke(ref.ref);
    if(ref.ref == table.lastRealInvocation)
    {
        CallaterFindNewLastInvocation(table.lastRealInvocation - 1);
    }
}

void CallaterStopRepeat(CallaterRef ref)
{
    table.repeatRates[ref.ref] = -1;
}

void CallaterSetFunc(CallaterRef ref, void(*func)(void*, CallaterRef))
{
    table.funcs[ref.ref] = func;
}

void CallaterSetRepeatRate(CallaterRef ref, float newRepeatRate)
{
    table.repeatRates[ref.ref] = newRepeatRate;
}

void CallaterSetGID(CallaterRef ref, uint64_t groupId)
{
    table.groupIDs[ref.ref] = groupId;
}

float CallaterGetRepeatRate(CallaterRef ref)
{
    return table.repeatRates[ref.ref];
}

uint64_t CallaterGetGID(CallaterRef ref)
{
    return table.groupIDs[ref.ref];
}

uint64_t CallaterGroupCount(uint64_t groupId)
{
    uint64_t count = 0;
    for(uint64_t i = 0 ; i <= table.lastRealInvocation ; i++)
    {
        count += (table.groupIDs[i] == groupId);
    }
    return count;
}

uint64_t CallaterGetGroupRefs(CallaterRef *refsOut, uint64_t groupId)
{
    uint64_t count = 0;
    for(uint64_t i = 0 ; i <= table.lastRealInvocation ; i++)
    {
        if(table.groupIDs[i] == groupId)
        {
            refsOut[count].ref = i;
            count += 1;
        }
    }
    return count;
}

static uint64_t CallaterCountNoop()
{
    uint64_t count = 0;
    for(uint64_t i = 0 ; i < table.count ; i++)
    {
        count += (table.funcs[i] == CallaterNoop);
    }
    return count;
}

void CallaterShrinkToFit()
{
    uint64_t newCap = table.lastRealInvocation + 1;
    CallaterReallocTable(newCap);
    
    if(table.count > newCap)
    {
        table.noopCount -= (table.count - newCap);
        table.count = newCap;
    }
    
#if defined(CALLATER_DEBUG)
    assert(CallaterCountNoop() == table.noopCount);
#endif
}

void CallaterDeinit()
{
    free(table.funcs);
    free(table.args);
    free((char*)table.invokeTimes - table.delaysPtrOffset);
    free(table.repeatRates);
    free(table.groupIDs);
    table = (CallaterTable){0};
}
