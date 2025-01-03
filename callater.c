#include "callater.h"
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <immintrin.h>

#ifdef _WIN32
#include <windows.h>
#include <sysinfoapi.h>
#endif

typedef struct CallaterTable
{
    size_t cap;
    size_t count;
    void(**funcs)(void*);
    void** args;
    float* delays;
    float lastUpdated;
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

static void *CallaterAlignedRealloc(void *ptr, size_t size, unsigned char alignment, unsigned char *offset)
{
    void *ret = realloc((unsigned char*)ptr - *offset, size + (alignment - 1));
    void *aligned = (void*)(((uintptr_t)ret + (alignment - 1)) & ~(alignment - 1));
    *offset = (char*)aligned - (char*)ret;
    return aligned;
}

void CallaterInit()
{
    table.lastUpdated = CallaterCurrentTime();
    
    table.count = 0;
    table.cap   = 64;
    table.funcs  = calloc(table.cap, sizeof(*table.funcs));
    table.args   = calloc(table.cap, sizeof(*table.args));
    table.delays = CallaterAlignedAlloc(table.cap * sizeof(*table.delays), 32, &table.delaysPtrOffset);
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
    for (size_t i = 0; i < table.count; i++)
    {
        if (table.funcs[i] == CallaterNoop)
            CallaterPopFunc(i);
    }
}

static void CallaterMaybeGrowTable()
{
    if (table.cap <= table.count)
    {
        table.cap *= 2;
        table.funcs  = realloc(table.funcs,  table.cap * sizeof(*table.funcs));
        table.args   = realloc(table.args,   table.cap * sizeof(*table.args));
        table.delays = CallaterAlignedRealloc(table.delays, table.cap * sizeof(*table.delays), 32, &table.delaysPtrOffset);
    }
}

static void CallaterInsertFunc(void(*func)(void*), void* arg, float delay)
{
    CallaterMaybeGrowTable();
    table.funcs[table.count] = func;
    table.delays[table.count] = delay;
    table.args[table.count] = arg;
    table.count += 1;
    
    if (table.count >= 1024)
        CallaterCleanupTable();
}

static void CallaterCallFunc(size_t idx)
{
    table.funcs[idx](table.args[idx]);
    table.funcs[idx] = CallaterNoop;
}

void CallaterUpdate()
{
    const float currentTime = CallaterCurrentTime();
    const float deltaTime = currentTime - table.lastUpdated;
    table.lastUpdated = currentTime;
    
    const size_t per8 = table.count / 8;
    const int remaining = table.count % 8;
    
    const __m256 zero = { 0 };
    const __m256 deltaVec = {
        deltaTime, deltaTime, deltaTime, deltaTime, deltaTime, deltaTime, deltaTime, deltaTime
    };
    
    size_t i;
    for (i = 0; i < per8; i++)
    {
        const size_t curVecIdx = i * 8;
        *(__m256*)(table.delays + curVecIdx) = _mm256_sub_ps(*(__m256*)(table.delays + curVecIdx), deltaVec);
        __m256 results = _mm256_cmp_ps(*(__m256*)(table.delays + curVecIdx), zero, _CMP_LE_OS);
        const int(*asInts)[8] = (void*)&results;
        for (int j = 0; j < 8; j++)
        {
            if ((*asInts)[j])
            {
                CallaterCallFunc(curVecIdx + j);
            }
        }
    }
    
    const size_t remainingIndex = i * 8;
    
    for(int j = 0 ; j < remaining ; j++)
    {
        table.delays[j + remainingIndex] -= deltaTime;
        if (table.delays[j + remainingIndex] <= 0)
        {
            CallaterCallFunc(j + remainingIndex);
        }
    }
}

void CallaterInvokeLater(void(*func)(void*), void* arg, float delay)
{
    CallaterInsertFunc(func, arg, delay);
}