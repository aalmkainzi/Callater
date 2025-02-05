#include <stdlib.h>
#include <string.h>
#include "callater.h"
#include "game.h"
#include "raymath.h"

typedef struct GameObjectGroup
{
    const char *name;
    uint32_t count, cap, elmSize;
    GameObject *objs;
    GameObjectCallbacks callbacks;
    uint32_t tag;
} GameObjectGroup;

typedef struct GameState
{
    GameObjectGroup *gameObjectGroups;
    uint32_t count, cap;
} GameState;

GameState gameState = { 0 };
uint32_t nextGameObjectTag = 0;
uint64_t nextId = 0;

static void *ArrayMaybeGrow(void *array, uint32_t *cap, uint32_t count, uint64_t elmSize)
{
    void *ret = array;
    if(count >= *cap)
    {
        uint32_t newCap = *cap * 2;
        ret = realloc(array, newCap * elmSize);
        *cap = newCap;
    }
    return ret;
}

__attribute__((constructor(101))) void GameStateInit()
{
    gameState = (GameState){0};
    uint32_t newCap = 16;
    gameState.gameObjectGroups = calloc(newCap, sizeof(gameState.gameObjectGroups[0]));
    gameState.cap = newCap;
}

void GameObjectGroupsMaybeGrow()
{
    if(gameState.count >= gameState.cap)
    {
        uint32_t newCap = gameState.cap * 2;
        gameState.gameObjectGroups = realloc(gameState.gameObjectGroups, newCap * sizeof(gameState.gameObjectGroups[0]));
        gameState.cap = newCap;
    }
}

void PushGameObjectGroup(uint32_t tag, GameObjectCallbacks callbacks, uint64_t gameObjSize, const char *typeName)
{
    GameObjectGroupsMaybeGrow();
    GameObjectGroup *newGroup = &gameState.gameObjectGroups[gameState.count];
    gameState.count += 1;
    
    *newGroup = (GameObjectGroup){.tag = tag, .callbacks = callbacks, .elmSize = gameObjSize, .name = typeName};
    uint32_t newGroupCap = 32;
    newGroup->objs = malloc(newGroupCap * gameObjSize);
    newGroup->cap = newGroupCap;
}

static GameObject *GameObjectAt(GameObjectGroup *group, uint32_t idx)
{
    unsigned char *elmBytes = (unsigned char*) group->objs;
    return (GameObject*)(elmBytes + (group->elmSize * idx));
}

GameObject *GetGameObject(GameObjectHandle handle)
{
    return GameObjectAt(&gameState.gameObjectGroups[handle.tag], handle.idx);
}

GameObjectHandle GetHandle(GameObject *go)
{
    return (GameObjectHandle){.tag = go->gameObjectHeader.tag, .idx = go->gameObjectHeader.index};
}

GameObject *AllocGameObject(uint32_t tag)
{
    GameObjectGroup *group = &gameState.gameObjectGroups[tag];
    
    for(uint32_t i = 0 ; i < group->count ; i++)
    {
        GameObject *go = GameObjectAt(group, i);
        if(go->gameObjectHeader.destroyed)
        {
            memset(go, 0, group->elmSize);
            *(uint32_t*)&go->gameObjectHeader.index = i;
            return go;
        }
    }
    
    group->objs = ArrayMaybeGrow(group->objs, &group->cap, group->count, group->elmSize);
    GameObject *go = GameObjectAt(group, group->count);
    memset(go, 0, group->elmSize);
    *(uint32_t*)&go->gameObjectHeader.index = group->count;
    group->count += 1;
    
    return go;
}

void DrawAllGameObjects()
{
    for(uint32_t i = 0 ; i < gameState.count ; i++)
    {
        GameObjectGroup *group = &gameState.gameObjectGroups[i];
        GameObjectHandle handle = {.tag = i};
        for(uint32_t j = 0 ; j < group->count ; j++)
        {
            handle.idx = j;
            if(!GetGameObject(handle)->gameObjectHeader.destroyed)
                group->callbacks.draw(handle);
        }
    }
}

void UpdateAllGameObjects()
{
    for(uint32_t i = 0 ; i < gameState.count ; i++)
    {
        GameObjectGroup *group = &gameState.gameObjectGroups[i];
        GameObjectHandle handle = {.tag = i};
        for(uint32_t j = 0 ; j < group->count ; j++)
        {
            handle.idx = j;
            if(!GetGameObject(handle)->gameObjectHeader.destroyed)
                group->callbacks.update(handle);
        }
    }
}

GameObject *CreateGameObject_ByTag(uint32_t tag, void *arg)
{
    GameObject *ret = AllocGameObject(tag);
    *(uint64_t*)&ret->gameObjectHeader.id = nextId++;
    *(uint32_t*)&ret->gameObjectHeader.tag = tag;
    GameObjectHandle handle = {.tag = tag, .idx = ret->gameObjectHeader.index};
    gameState.gameObjectGroups[tag].callbacks.init(handle, arg);
    return ret;
}

GameObject *CreateGameObject_ByName(const char *name, void *arg)
{
    return CreateGameObject_ByTag(NameToTag(name), arg);
}

void DestroyGameObject(GameObject *go)
{
    GameObjectGroup *group = &gameState.gameObjectGroups[go->gameObjectHeader.tag];
    CallaterCancelGID(go->gameObjectHeader.id);
    if(go->gameObjectHeader.index == group->count - 1)
    {
        group->count -= 1;
        group->callbacks.deinit(GetHandle(go));
    }
    else
    {
        go->gameObjectHeader.destroyed = true;
        group->callbacks.deinit(GetHandle(go));
    }
}

uint32_t GameObjectTag(GameObjectHandle handle)
{
    return GetGameObject(handle)->gameObjectHeader.tag;
}

uint32_t NameToTag(const char *typeName)
{
    for(uint32_t i = 0 ; i < gameState.count ; i++)
    {
        if(strcmp(gameState.gameObjectGroups[i].name, typeName) == 0)
            return i;
    }
    return (uint32_t)-1;
}

const char *TagToName(uint32_t tag)
{
    return gameState.gameObjectGroups[tag].name;
}

bool CirclesOverlapping(Circle a, Circle b)
{
    float dist = Vector2Distance(a.center, b.center);
    return dist <= (a.radius + b.radius);
}

static uint32_t rng_state = 1;

void SeedRNG(uint32_t seed) {
    rng_state = (seed != 0) ? seed : 1;
}

uint32_t RandomUInt()
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

float RandomFloat01()
{
    uint32_t bits = (127u << 23) | (RandomUInt() & 0x7FFFFF);
    
    union {
        uint32_t asInt;
        float asFloat;
    } pun;
    pun.asInt = bits;
    return pun.asFloat - 1.0f;
}

float RandomFloat(float min, float max)
{
    return RandomFloat01() * (max - min) + min;
}
