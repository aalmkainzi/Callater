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

#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER

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

/*

This needs proper designing...
First of all, users should be able to refer to the invocation by the CallaterRef.
The CallaterRef returned from the Invoke functions must not be invalidated if the invocation is still there.
meaning, it either can't be an index (because we're doing sorted array where indexes shift).
Ideas:
1) don't shift when removing, instead put NoOp there
2) use linked list, where CallaterRef is pointer to the node
3) give each entry an id, that id is the CallaterRef (using the ref then is O(n) because it must be searched first)

*/

typedef struct CallaterInvokeData
{
    uint64_t groupId;
} CallaterInvokeData;

typedef struct OrderedArray
{
    uint64_t count, cap;
    float *elms;
    unsigned char offset;
} OrderedArray;

static void OrderedArrayInit(OrderedArray *oa, uint64_t initCap);
static uint64_t OrderedArrayPut(OrderedArray *oa, float newVal);
static void OrderedArrayPop(OrderedArray *oa, uint64_t index);
static void OrderedArrayRealloc(OrderedArray *oa, uint64_t newCap);

typedef struct PQEntry
{
    void(*func)(void*, CallaterRef);
    void *arg;
    
    uint64_t groupId;
    float repeatRate;
    float invokeTime;
} PQEntry;

typedef struct PriorityQueue
{
    uint64_t count, cap;
    PQEntry *elms;
} PriorityQueue;

static void PriorityQueueInit(PriorityQueue *pq, uint64_t initCap);
static void PriorityQueuePush(PriorityQueue *pq, PQEntry elm);
static PQEntry PriorityQueuePop(PriorityQueue *pq);

typedef struct CallaterTable
{
    uint64_t startSec;
    uint64_t clockFreq;
    
    uint64_t cap;
    uint64_t count;
    
    void(**funcs)(void*, CallaterRef);
    void **args;
    CallaterInvokeData *invokeData;
    
    OrderedArray nonRepeatInvokes;
    
    PriorityQueue repeatInvokesQueue;
    
    float minInvokeTime;
    float lastUpdated;
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
    
    uint64_t initCap = 64;
    table.funcs  = calloc(initCap, sizeof(*table.funcs));
    table.args   = calloc(initCap, sizeof(*table.args));
    OrderedArrayInit(&table.nonRepeatInvokes, initCap);
    table.invokeData = malloc(initCap * sizeof(*table.invokeData));
    table.minInvokeTime = INFINITY;
    
    table.cap = initCap;
    table.count = 0;
}

static void CallaterNoop(void *arg, CallaterRef ref)
{
    (void)arg;
    (void)ref;
}

static void CallaterRemovePause(uint64_t index);

// static void CallaterPopInvoke(uint64_t idx)
// {
//     table.noopCount += (table.funcs[idx] != CallaterNoop);
//     table.funcs[idx] = CallaterNoop;
//     table.args[idx] = NULL;
//     table.invokeTimes[idx] = INFINITY;
//     table.invokeData[idx].groupId = (uint64_t)-1;
//     table.invokeData[idx].repeatRate = INFINITY;
//     
//     if(table.invokeData[idx].pausedIndex != (uint64_t)-1)
//     {
//         CallaterRemovePause(table.invokeData[idx].pausedIndex);
//     }
// }

static void CallaterReallocTable(uint64_t newCap)
{
    table.funcs      = realloc(table.funcs,      newCap * sizeof(*table.funcs));
    table.args       = realloc(table.args,       newCap * sizeof(*table.args));
    table.invokeData = realloc(table.invokeData, newCap * sizeof(*table.invokeData));
    OrderedArrayRealloc(&table.nonRepeatInvokes, newCap);
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
    if(signbit(table.invokeData[idx].repeatRate))
    {
        CallaterPopInvoke(idx);
    }
    else
    {
        table.invokeTimes[idx] = curTime + table.invokeData[idx].repeatRate;
    }
}

static void CallaterFindNewLastInvocation(uint64_t startFrom)
{
    uint64_t lastInvoke = -1;
    for(uint64_t i = startFrom ; i != (uint64_t)-1 ; i--)
    {
        if(table.funcs[i] != CallaterNoop)
        {
            lastInvoke = i;
            break;
        }
    }
    table.count = lastInvoke + 1;
}

static void CallaterFindNewMinInvokeTime()
{
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

static void CallaterTick(float curTime)
{
    const __m256 curTimeVec = _mm256_set1_ps(curTime);
    
    uint64_t i;
    for(i = 0 ; i + 7 < table.count ; i += 8)
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
    
    CallaterFindNewMinInvokeTime();
    
    if(table.funcs[table.count - 1] == CallaterNoop)
    {
        CallaterFindNewLastInvocation(table.count - 1);
    }
}

void CallaterUpdate()
{
    float curTime = CallaterCurrentTime();
    table.lastUpdated = curTime;
    
    // do we even need the second term? minInvokeTime should be enough
    if(curTime < table.minInvokeTime || table.count == 0)
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
    if(table.count >= table.cap)
    {
        CallaterMaybeGrowTable();
    }
    
    float curTime = CallaterCurrentTime();
    uint64_t index = OrderedArrayPut(&table.nonRepeatInvokes, curTime + delay);
    
    table.funcs     [index] = func;
    table.args      [index] = arg;
    table.invokeData[index].groupId = groupId;
    table.minInvokeTime = table.nonRepeatInvokes.elms[table.nonRepeatInvokes.count - 1];
    
    return (CallaterRef){index};
}

CallaterRef CallaterInvokeRepeat(void(*func)(void*, CallaterRef), void *arg, float firstDelay, float repeatRate)
{
    return CallaterInvokeRepeatGID(func, arg, firstDelay, repeatRate, (uint64_t)-1);
}

CallaterRef CallaterInvokeRepeatGID(void(*func)(void*, CallaterRef), void *arg, float firstDelay, float repeatRate, uint64_t groupId)
{
    PQEntry newRepeatInvoke = {
        .invokeTime = CallaterCurrentTime() + firstDelay,
        .repeatRate = repeatRate,
        .arg = arg,
        .func = func,
        .groupId = groupId
    };
    
    return ret;
}

float CallaterInvokesAfter(CallaterRef ref)
{
    return table.invokeTimes[ref.ref] - CallaterCurrentTime();
}

void CallaterCancelGID(uint64_t groupId)
{
    for(uint64_t i = 0 ; i < table.count ; i++)
    {
        if(table.invokeData[i].groupId == groupId)
        {
            CallaterCancel((CallaterRef){i});
        }
    }
}

void CallaterCancelFunc(void(*func)(void*, CallaterRef))
{
    for(uint64_t i = 0 ; i < table.count ; i++)
    {
        if(table.funcs[i] == func)
        {
            CallaterCancel((CallaterRef){i});
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
    return CALLATER_REF_ERR;
}

void CallaterCancel(CallaterRef ref)
{
    bool isMinInvokeTime = table.invokeTimes[ref.ref] == table.minInvokeTime;
    bool isLastInvocation = ref.ref == table.count - 1;
    
    CallaterPopInvoke(ref.ref);
    
    if(isLastInvocation)
    {
        CallaterFindNewLastInvocation(table.count - 1);
    }
    if(isMinInvokeTime)
    {
        CallaterFindNewMinInvokeTime();
    }
}

void CallaterStopRepeat(CallaterRef ref)
{
    table.invokeData[ref.ref].repeatRate = -1;
}

void CallaterSetRepeatRate(CallaterRef ref, float newRepeatRate)
{
    table.invokeData[ref.ref].repeatRate = newRepeatRate;
}

float CallaterGetRepeatRate(CallaterRef ref)
{
    return table.invokeData[ref.ref].repeatRate;
}

void CallaterSetFunc(CallaterRef ref, void(*func)(void*, CallaterRef))
{
    table.funcs[ref.ref] = func;
}

typeof(void(*)(void*, CallaterRef)) CallaterGetFunc(CallaterRef ref)
{
    return table.funcs[ref.ref];
}

void CallaterSetArg(CallaterRef ref, void *arg)
{
    table.args[ref.ref] = arg;
}

void *CallaterGetArg(CallaterRef ref)
{
    return table.args[ref.ref];
}

void CallaterSetGID(CallaterRef ref, uint64_t groupId)
{
    table.invokeData[ref.ref].groupId = groupId;
}

uint64_t CallaterGetGID(CallaterRef ref)
{
    return table.invokeData[ref.ref].groupId;
}

uint64_t CallaterGroupCount(uint64_t groupId)
{
    uint64_t count = 0;
    for(uint64_t i = 0 ; i < table.count ; i++)
    {
        count += (table.invokeData[i].groupId == groupId);
    }
    return count;
}

uint64_t CallaterGetGroupRefs(CallaterRef *refsOut, uint64_t groupId)
{
    uint64_t count = 0;
    for(uint64_t i = 0 ; i < table.count ; i++)
    {
        if(table.invokeData[i].groupId == groupId)
        {
            refsOut[count].ref = i;
            count += 1;
        }
    }
    return count;
}

void CallaterPause(CallaterRef ref)
{
    if(table.invokeData[ref.ref].pausedIndex != (uint64_t)-1)
        return;
    
    CallaterPauseArray *pauseArray = &table.pausedInvokes;
    if(pauseArray->count >= pauseArray->cap)
    {
        pauseArray->cap *= 2;
        pauseArray->pausedInvokes =
        realloc(
            pauseArray->pausedInvokes,
            pauseArray->cap * sizeof(*pauseArray->pausedInvokes)
        );
    }
    
    float invokeTime = table.invokeTimes[ref.ref];
    float delay = table.invokeTimes[ref.ref] - table.lastUpdated;
    
    pauseArray->pausedInvokes[pauseArray->count] = (CallaterPausedInvoke){.ref = ref, .delay = delay};
    table.invokeData[ref.ref].pausedIndex = pauseArray->count;
    table.invokeTimes[ref.ref] = INFINITY;
    pauseArray->count += 1;
    if(table.minInvokeTime == invokeTime)
    {
        CallaterFindNewMinInvokeTime();
    }
}

void CallaterPauseGID(uint64_t groupId)
{
    for(uint64_t i = 0 ; i < table.count ; i++)
    {
        if(table.invokeData[i].groupId == groupId)
        {
            CallaterPause((CallaterRef){i});
        }
    }
}

static void CallaterRemovePause(uint64_t index)
{
    CallaterPauseArray *pauseArray = &table.pausedInvokes;
    pauseArray->pausedInvokes[index] = pauseArray->pausedInvokes[pauseArray->count - 1];
    CallaterRef movedRef = pauseArray->pausedInvokes[index].ref;
    table.invokeData[movedRef.ref].pausedIndex = index;
    pauseArray->count -= 1;
}

void CallaterResume(CallaterRef ref)
{
    CallaterPauseArray *pauseArray = &table.pausedInvokes;
    uint64_t index = table.invokeData[ref.ref].pausedIndex;
    table.invokeData[ref.ref].pausedIndex = (uint64_t)-1;
    if(index != (uint64_t)-1)
    {
        CallaterPausedInvoke pi = pauseArray->pausedInvokes[index];
        table.invokeTimes[ref.ref] = pi.delay + CallaterCurrentTime();
        
        CallaterRemovePause(index);
        
        // TODO only do this if this is lower than table.minInvokeTime
        CallaterFindNewMinInvokeTime();
        return;
    }
}

void CallaterResumeGID(uint64_t groupId)
{
    CallaterPauseArray *pauseArray = &table.pausedInvokes;
    for(uint64_t i = 0 ; i < pauseArray->count ; i++)
    {
        if(CallaterGetGID(pauseArray->pausedInvokes[i].ref) == groupId)
        {
            CallaterResume(pauseArray->pausedInvokes[i].ref);
            i -= 1;
        }
    }
}

bool CallaterRefError(CallaterRef ref)
{
    return ref.ref == (uint64_t)-1;
}

uint64_t CallaterCountNoop()
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
    uint64_t newCap = table.count + 1;
    CallaterReallocTable(newCap);
    
#if defined(DEBUG)
    assert(CallaterCountNoop() == table.noopCount);
#endif
}

void CallaterDeinit()
{
    free(table.funcs);
    free(table.args);
    free((char*)table.invokeTimes - table.delaysPtrOffset);
    free(table.invokeData);
    free(table.pausedInvokes.pausedInvokes);
    table = (CallaterTable){0};
}

// PriorityQueue implementation:

static void PriorityQueueInit(PriorityQueue *pq, uint64_t initCap)
{
    pq->count = 0;
    pq->cap = initCap;
    pq->elms = calloc(pq->cap + 1, sizeof(*pq->elms));
}

static void Swap(float *f1, float *f2)
{
    float tmp = *f1;
    *f1 = *f2;
    *f2 = tmp;
}

static void HUp(PriorityQueue *pq, uint64_t index)
{
    uint64_t parentIdx = index / 2;
    while(index > 0 && pq->elms[parentIdx] > pq->elms[index])
    {
        Swap(&pq->elms[parentIdx], &pq->elms[index]);
        index = parentIdx;
        parentIdx = index / 2;
    }
}

static void PriorityQueuePush(PriorityQueue *pq, PQEntry f)
{
    if(pq->count >= pq->cap - 1)
    {
        pq->cap *= 2;
        pq->elms = realloc(pq->elms, (pq->cap + 1) * sizeof(*pq->elms));
    }
    pq->elms[pq->count + 1] = f;
    HUp(pq, pq->count + 1);
    pq->count += 1;
}

static void HDown(PriorityQueue *pq, uint64_t index)
{
    while(1)
    {
        uint64_t right = index * 2 + 1;
        uint64_t left  = index * 2;
        uint64_t smallest = index;
        
        if(right <= pq->count)
        {
            if(pq->elms[right] < pq->elms[smallest])
            {
                smallest = right;
            }
        }
        if(left <= pq->count)
        {
            if(pq->elms[left] < pq->elms[smallest])
            {
                smallest = left;
            }
        }
        
        if(smallest != index)
        {
            Swap(&pq->elms[smallest], &pq->elms[index]);
            index = smallest;
        }
        else
        {
            break;
        }
    }
}

static float PriorityQueuePop(PriorityQueue *pq)
{
    float minVal = pq->elms[1];
    pq->elms[1] = pq->elms[pq->count];
    pq->count -= 1;
    HDown(pq, 1);
    return minVal;
}

static void PriorityQueueFree(PriorityQueue *pq)
{
    free(pq->elms);
}

// OrderedArray implementation

static void OrderedArrayInit(OrderedArray *oa, uint64_t initCap)
{
    oa->count = 0;
    oa->cap = initCap;
    oa->elms = CallaterAlignedAlloc(initCap, 16, &oa->offset);
}

static void OrderedArrayRealloc(OrderedArray *oa, uint64_t newCap)
{
    oa->elms = (float *)realloc(oa->elms, oa->cap * sizeof(float));
    oa->elms = CallaterAlignedRealloc(oa->elms, newCap, oa->cap * sizeof(oa->elms[0]), 16, &oa->offset)
    oa->cap = newCap;
}

static uint64_t OrderedArrayPut(OrderedArray *oa, float elm)
{
    uint64_t left = 0
    uint64_t right = oa->count;
    while (left < right)
    {
        uint64_t mid = left + (right - left) / 2;
        if (oa->elms[mid] < elm)
        {
            left = mid + 1;
        }
        else
        {
            right = mid;
        }
    }
    
    memmove(&oa->elms[left + 1], &oa->elms[left], (oa->count - left) * sizeof(float));
    
    oa->elms[left] = elm;
    oa->count++;
    
    return left;
}

static void OrderedArrayPop(OrderedArray *oa, uint64_t index)
{
    table.funcs[index] = CallaterNoop;
}

static void OrderedArrayFree(OrderedArray *oa)
{
    free((unsigned char*) oa->elms - oa->offset);
    oa->elms = NULL;
    oa->count = 0;
    oa->cap = 0;
}
