#include "callater.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>
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
    float* times;
    float lastUpdated;
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

void CallaterInit()
{
    table.lastUpdated = CallaterCurrentTime();
    
    table.count = 0;
    table.cap = 64;
    table.funcs = calloc(table.cap, sizeof(*table.funcs));
    table.args  = calloc(table.cap, sizeof(*table.args));
    table.times = calloc(table.cap, sizeof(*table.times));
}

static void CallaterNoop(void* arg)
{
    (void)arg;
}

static void CallaterPopFunc(size_t idx)
{
    table.funcs[idx] = table.funcs[table.count - 1];
    table.times[idx] = table.times[table.count - 1];
    table.args[idx] = table.args[table.count - 1];
    
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
        table.funcs = realloc(table.funcs, table.cap * sizeof(*table.funcs));
        table.args  = realloc(table.args,  table.cap * sizeof(*table.args));
        table.times = realloc(table.times, table.cap * sizeof(*table.times));
    }
}

static void CallaterInsertFunc(void(*func)(void*), void* arg, float delay)
{
    CallaterMaybeGrowTable();
    table.funcs[table.count] = func;
    table.times[table.count] = delay;
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
    float currentTime = CallaterCurrentTime();
    float deltaTime = currentTime - table.lastUpdated;
    table.lastUpdated = currentTime;
    
    size_t per8 = table.count / 8;
    size_t remaining = table.count % 8;
    
    __m256 zero = { 0 };
    __m256 deltaVec = {
        deltaTime, deltaTime, deltaTime, deltaTime, deltaTime, deltaTime, deltaTime, deltaTime
    };
    
    size_t i;
    for (i = 0; i < per8; i++)
    {
        const size_t curVecIdx = i * 8;
        *(__m256*)(table.times + curVecIdx) = _mm256_sub_ps(*(__m256*)(table.times + curVecIdx), deltaVec);
        __m256 results = _mm256_cmp_ps(*(__m256*)(table.times + curVecIdx), zero, _CMP_LE_OS);
        const int(*asInts)[8] = (void*)&results;
        for (int j = 0; j < 8; j++)
        {
            if ((*asInts)[j])
            {
                CallaterCallFunc(curVecIdx + j);
            }
        }
    }
    
    for (size_t j = i * 8; j < table.count; j++)
    {
        table.times[j] -= deltaTime;
        if (table.times[j] <= 0)
        {
            CallaterCallFunc(j);
        }
    }
}

void CallaterInvokeLater(void(*func)(void*), void* arg, float delay)
{
    CallaterInsertFunc(func, arg, delay);
}