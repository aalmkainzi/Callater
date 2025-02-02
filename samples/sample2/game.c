#include <stdlib.h>
#include <string.h>
#include "game.h"

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
    ArrayMaybeGrow((void**)&gameObjectGroup->objs, &gameObjectGroup->cap, gameObjectGroup->count, gameObjectGroup->elmSize);
    GameObject *ret = GameObjectAt(gameObjectGroup, gameObjectGroup->count);
    ret->gameObjectHeader.id = nextId++;
    gameObjectGroup->count += 1;
    
    memset(ret, 0, gameObjectGroup->elmSize);
    return ret;
}

void DrawAllGameObjects()
{
    for(uint64_t i = 0 ; i < gameState.count ; i++)
    {
        GameObjectGroup *group = &gameState.gameObjectGroups[i];
        for(uint64_t j = 0 ; j < group->count ; j++ )
        {
            group->callbacks.draw(GameObjectAt(group, j));
        }
    }
}

void UpdateAllGameObjects()
{
    for(uint64_t i = 0 ; i < gameState.count ; i++)
    {
        GameObjectGroup *group = &gameState.gameObjectGroups[i];
        for(uint64_t j = 0 ; j < group->count ; j++ )
        {
            group->callbacks.update(GameObjectAt(group, j));
        }
    }
}

GameObject *CreateGameObject_ByTag(uint32_t tag)
{
    GameObject *ret = AllocGameObject(tag);
    gameState.gameObjectGroups[tag].callbacks.init(ret);
    return ret;
}

GameObject *CreateGameObject_ByName(const char *name)
{
    return CreateGameObject_ByTag(NameToTag(name));
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
