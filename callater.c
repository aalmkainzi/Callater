#include "callater.h"
#include <stdlib.h>
#include <time.h>

typedef struct FuncsToCall
{
    size_t cap;
    size_t count;
    typeof(void(void*)) **funcs;
    void **args;
    float *times;
} FuncsToCall;

FuncsToCall table;
float lastUpdated;

float GetCurrentTime()
{
    struct timespec ts;
    #ifdef __MINGW32__
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
    #else
    timespec_get(&ts, 1);
    #endif
    return ts.tv_sec + (ts.tv_nsec / 1000000000.0f);
}

void CallaterInit()
{
    lastUpdated = GetCurrentTime();

    table.count = 0;
    table.cap = 64;
    table.funcs = calloc(table.cap, sizeof(*table.funcs));
    table.args  = calloc(table.cap, sizeof(*table.args));
    table.times = calloc(table.cap, sizeof(*table.times));
}

static void MaybeGrowTable()
{
    if(table.cap <= table.count)
    {
        table.cap *= 2;
        table.funcs = realloc(table.funcs,    table.cap * sizeof(*table.funcs));
        table.args  = realloc(table.args,     table.cap * sizeof(*table.args));
        table.times = realloc(table.times,    table.cap * sizeof(*table.times));
    }
}

static void InsertFunc(void(*func)(void*), void *arg, float delay)
{
    MaybeGrowTable();
    table.funcs[table.count] = func;
    table.times[table.count] = delay;
    table.args[table.count]  = arg;
    table.count += 1;
}

static void CallFunc(size_t idx)
{
    table.funcs[idx](table.args[idx]);

    table.funcs[idx] = table.funcs[table.count - 1];
    table.times[idx] = table.times[table.count - 1];
    table.args[idx]  = table.args[table.count - 1];

    table.count -= 1;
}

void CallaterUpdate()
{
    float timeSinceLastUpdate = GetCurrentTime() - lastUpdated;

    for(size_t i = 0 ; i < table.count ; i++)
    {
        table.times[i] -= timeSinceLastUpdate;
    }

    for(size_t i = 0 ; i < table.count ; i++)
    {
        if(table.times[i] <= 0)
        {
            CallFunc(i);
        }
    }
}

void InvokeLaterArg(void(*func)(void*), void *arg, float delay)
{
    InsertFunc(func, arg, delay);
}

void InvokeLater(void(*func)(), float delay)
{
    InsertFunc(func, NULL, delay);
}