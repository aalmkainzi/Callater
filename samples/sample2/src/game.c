#include <stdlib.h>
#include <string.h>
#include "callater.h"
#include "game.h"
#include "raymath.h"

typedef struct GameState
{
    GameObjectGroup *gameObjectGroups;
    uint64_t count, cap;
} GameState;

GameState gameState = { 0 };
uint32_t nextGameObjectTag = 0;
uint64_t nextId = 0;

static void ArrayMaybeGrow(void **array, uint64_t *cap, uint64_t count, uint64_t elmSize)
{
    if(count >= *cap)
    {
        uint64_t newCap = *cap * 2;
        *array = realloc(*array, newCap * elmSize);
        *cap = newCap;
    }
}

__attribute__((constructor(101))) void GameStateInit()
{
    gameState = (GameState){0};
    uint64_t newCap = 16;
    gameState.gameObjectGroups = calloc(newCap, sizeof(gameState.gameObjectGroups[0]));
    gameState.cap = newCap;
}

void GameObjectGroupsMaybeGrow()
{
    if(gameState.count >= gameState.cap)
    {
        uint64_t newCap = gameState.cap * 2;
        gameState.gameObjectGroups = realloc(gameState.gameObjectGroups, newCap * sizeof(gameState.gameObjectGroups[0]));
        gameState.cap = newCap;
    }
}

static GameObject *GameObjectAt(GameObjectGroup *group, uint64_t idx)
{
    unsigned char *elmBytes = (unsigned char*) group->objs;
    return (GameObject*)(elmBytes + (group->elmSize * idx));
}

void PushGameObjectGroup(uint32_t tag, GameObjectCallbacks callbacks, uint64_t gameObjSize, const char *typeName)
{
    GameObjectGroupsMaybeGrow();
    GameObjectGroup *newGroup = &gameState.gameObjectGroups[gameState.count];
    gameState.count += 1;
    
    *newGroup = (GameObjectGroup){.tag = tag, .callbacks = callbacks, .elmSize = gameObjSize, .name = typeName};
    uint64_t newGroupCap = 32;
    newGroup->objs = malloc(newGroupCap * gameObjSize);
    newGroup->cap = newGroupCap;
}

GameObject *AllocGameObject(uint32_t tag)
{
    GameObjectGroup *gameObjectGroup = &gameState.gameObjectGroups[tag];
    GameObject *foundSlot = NULL;
    for(uint64_t i = 0 ; i <gameObjectGroup->count ; i++)
    {
        GameObject *go = GameObjectAt(gameObjectGroup, i);
        if(go->gameObjectHeader.destroy)
        {
            foundSlot = go;
            memset(foundSlot, 0, gameObjectGroup->elmSize);
            return foundSlot;
        }
    }
    
    ArrayMaybeGrow((void**)&gameObjectGroup->objs, &gameObjectGroup->cap, gameObjectGroup->count, gameObjectGroup->elmSize);
    foundSlot = GameObjectAt(gameObjectGroup, gameObjectGroup->count);
    gameObjectGroup->count += 1;
    
    memset(foundSlot, 0, gameObjectGroup->elmSize);
    return foundSlot;
}

void DrawAllGameObjects()
{
    for(uint64_t i = 0 ; i < gameState.count ; i++)
    {
        GameObjectGroup *group = &gameState.gameObjectGroups[i];
        for(uint64_t j = 0 ; j < group->count ; j++)
        {
            GameObject *go = GameObjectAt(group, j);
            if(!go->gameObjectHeader.destroy)
                group->callbacks.draw(go);
        }
    }
}

void UpdateAllGameObjects()
{
    for(uint64_t i = 0 ; i < gameState.count ; i++)
    {
        GameObjectGroup *group = &gameState.gameObjectGroups[i];
        for(uint64_t j = 0 ; j < group->count ; j++)
        {
            GameObject *go = GameObjectAt(group, j);
            if(!go->gameObjectHeader.destroy)
                group->callbacks.update(GameObjectAt(group, j));
        }
    }
}

GameObject *CreateGameObject_ByTag(uint32_t tag, void *arg)
{
    GameObject *ret = AllocGameObject(tag);
    *(uint64_t*)&ret->gameObjectHeader.id = nextId++;
    *(uint32_t*)&ret->gameObjectHeader.tag = tag;
    gameState.gameObjectGroups[tag].callbacks.init(ret, arg);
    return ret;
}

GameObject *CreateGameObject_ByName(const char *name, void *arg)
{
    return CreateGameObject_ByTag(NameToTag(name), arg);
}

void DestroyGameObject(GameObject *go)
{
    GameObjectGroup *group = &gameState.gameObjectGroups[go->gameObjectHeader.tag];
    group->callbacks.deinit(go);
    go->gameObjectHeader.destroy = true;
    CallaterCancelGID(go->gameObjectHeader.id);
}

uint32_t GameObjectTag(GameObject *go)
{
    return go->gameObjectHeader.tag;
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
