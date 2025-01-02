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
    float* times;
} FuncsToCall;

FuncsToCall table;
float lastUpdated;

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

static void InsertFunc(void(*func)(void*), void* arg, float delay)
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
    float currentTime = CallaterCurrentTime();
    float deltaTime = currentTime - lastUpdated;
    lastUpdated = currentTime;

    for (size_t i = 0; i < table.count; i++)
    {
        table.times[i] -= deltaTime;
    }

    size_t per8 = table.count / 8;
    size_t remaining = table.count % 8;

    __m256 zero = { 0 };
    size_t i;
    for (i = 0; i < per8; i++)
    {
        __m256 floats;
        memcpy(&floats, table.times + (i * 16), sizeof(__m256));
        __m256 results = _mm256_cmp_ps(zero, zero, _CMP_LE_OS);
        int32_t* asInts = (int32_t*)&results;

        for (int j = 0; j < 8; j++)
        {
            if (asInts[j])
            {
                CallAndPop(i * 8 + j);
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

void InvokeLaterArg(void(*func)(void*), void* arg, float delay)
{
    InsertFunc(func, arg, delay);
}

void InvokeLater(void(*func)(), float delay)
{
    InsertFunc(func, NULL, delay);
}
