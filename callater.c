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
    float *originalDelays;
    // float startTime;
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
    // table.startTime = CallaterCurrentTime();
    
    table.count = 0;
    table.cap   = 64;
    table.funcs  = calloc(table.cap, sizeof(*table.funcs));
    table.args   = calloc(table.cap, sizeof(*table.args));
    table.invokeTimes = CallaterAlignedAlloc(table.cap * sizeof(*table.invokeTimes), 32, &table.delaysPtrOffset);
    table.originalDelays = malloc(table.cap * sizeof(*table.originalDelays));
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

void CallaterUpdate()
{
    typedef struct Entry
    {
        float invokeTime;
        void *arg;
        void(*func)(void*);
    } Entry;
    
    float curTime = CallaterCurrentTime();
    
    Entry *repeatEntries = malloc(sizeof(Entry) * table.count);
    size_t nbRepeat = 0;
    
    for(size_t i = table.count - 1 ; i != (size_t)-1 ; i--)
    {
        if(table.invokeTimes[i] > curTime)
        {
            break;
        }
        
        table.funcs[i](table.args[i]);
        table.count -= 1;
        
        if(table.originalDelays[i] >= 0)
        {
            repeatEntries[nbRepeat++] = (Entry){
                .invokeTime = table.originalDelays[i],
                .arg        = table.args[i],
                .func       = table.funcs[i]
            };
        }
    }
    
    for(size_t i = 0 ; i < nbRepeat ; i++)
    {
        CallaterInvokeRepeat(repeatEntries[i].func, repeatEntries[i].arg, repeatEntries[i].invokeTime, repeatEntries[i].invokeTime);
    }
    
    free(repeatEntries);
}

static void CallaterInsertFuncAt(void(*func)(void*), void *arg, float invokeTime, float originalDelay, size_t idx)
{
    // shift buffers by 1 to the right
    memmove(table.funcs          + idx + 1, table.funcs          + idx, (table.count - idx) * sizeof(*table.funcs));
    memmove(table.args           + idx + 1, table.args           + idx, (table.count - idx) * sizeof(*table.args));
    memmove(table.invokeTimes    + idx + 1, table.invokeTimes    + idx, (table.count - idx) * sizeof(*table.invokeTimes));
    memmove(table.originalDelays + idx + 1, table.originalDelays + idx, (table.count - idx) * sizeof(*table.originalDelays));
    
    table.invokeTimes[idx] = invokeTime;
    table.funcs[idx] = func;
    table.args[idx] = arg;
    table.originalDelays[idx] = originalDelay;
    
    table.count += 1;
}

static void CallaterInsertFunc(void(*func)(void*), void *arg, float invokeTime, float originalDelay)
{
    size_t begin = 0;
    size_t end = table.count;
    size_t mid = (end + begin) / 2;
    
    while(begin < end)
    {
        mid = (end + begin) / 2;
        if(invokeTime < table.invokeTimes[mid])
        {
            begin = mid + 1;
        }
        else if(invokeTime > table.invokeTimes[mid])
        {
            end = mid;
        }
        else
        {
            break;
        }
    }
    
    CallaterInsertFuncAt(func, arg, invokeTime, originalDelay, begin);
}

void CallaterInvoke(void(*func)(void*), void* arg, float delay)
{
    CallaterMaybeGrowTable();
    
    float invokeTime = delay + CallaterCurrentTime();
    
    CallaterInsertFunc(func, arg, invokeTime, -delay - 1.0f);
}

void CallaterInvokeNull(void(*func)(void*), float delay)
{
    CallaterInvoke(func, NULL, delay);
}

void CallaterInvokeRepeat(void(*func)(void*), void *arg, float firstDelay, float repeatRate)
{
    CallaterMaybeGrowTable();
    
    float invokeTime = firstDelay + CallaterCurrentTime();
    
    CallaterInsertFunc(func, arg, invokeTime, repeatRate);
}

void CallaterShrinkToFit()
{
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
