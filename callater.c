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

typedef struct FuncsToCall
{
    size_t cap;
    size_t count;
    void(**funcs)(void*);
    void** args;
    double* times;
} FuncsToCall;

FuncsToCall table;
double lastUpdated;

double CallaterCurrentTime()
{
#ifdef _WIN32
    return GetTickCount64() / 1000.0;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return ts.tv_sec + ts.tv_nsec / 1000000000.0;
#endif
}

void CallaterInit()
{
    lastUpdated = CallaterCurrentTime();

    table.count = 0;
    table.cap = 64;
    table.funcs = calloc(table.cap, sizeof(*table.funcs));
    table.args = calloc(table.cap, sizeof(*table.args));
    table.times = calloc(table.cap, sizeof(*table.times));
}

static void MaybeGrowTable()
{
    if (table.cap <= table.count)
    {
        table.cap *= 2;
        table.funcs = realloc(table.funcs, table.cap * sizeof(*table.funcs));
        table.args = realloc(table.args, table.cap * sizeof(*table.args));
        table.times = realloc(table.times, table.cap * sizeof(*table.times));
    }
}

static void InsertFunc(void(*func)(void*), void* arg, double delay)
{
    MaybeGrowTable();
    table.funcs[table.count] = func;
    table.times[table.count] = delay;
    table.args[table.count] = arg;
    table.count += 1;
}

static void CallAndPop(size_t idx)
{
    table.funcs[idx](table.args[idx]);

    table.funcs[idx] = table.funcs[table.count - 1];
    table.times[idx] = table.times[table.count - 1];
    table.args[idx] = table.args[table.count - 1];

    table.count -= 1;
}

void CallaterUpdate()
{
    double currentTime = CallaterCurrentTime();
    double deltaTime = currentTime - lastUpdated;
    lastUpdated = currentTime;

    for (size_t i = 0; i < table.count; i++)
    {
        table.times[i] -= deltaTime;
    }

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
        *(__m256*)(table.times + curVecIdx) = _mm256_sub_ps (*(__m256*)(table.times + curVecIdx), deltaVec);
        
        __m256 results = _mm256_cmp_ps  (*(__m256*)(table.times + curVecIdx), zero, _CMP_LE_OS);
        
        for (int j = 0; j < 8; j++)
        {
            if (results[j])
            {
                CallAndPop(curVecIdx + j);
            }
        }
    }

    for (size_t j = i; j < table.count; j++)
    {
        if (table.times[j] <= 0)
        {
            CallAndPop(j);
        }
    }
}

void InvokeLaterArg(void(*func)(void*), void* arg, double delay)
{
    InsertFunc(func, arg, delay);
}

void InvokeLater(void(*func)(), double delay)
{
    InsertFunc(func, NULL, delay);
}
