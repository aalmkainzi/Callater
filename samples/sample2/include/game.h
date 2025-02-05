#ifndef GAME_H
#define GAME_H

#include <malloc.h>
#include <stddef.h>
#include <stdint.h>
#include <stdalign.h>
#include "raylib.h"

#define TYPE_PUN(x, to) \
((union { typeof(to) AsTo; typeof(x) AsOld; }){.AsOld = x}.AsTo)

#define DECL_GAMEOBJECT(name, ...) \
typedef struct name \
{ \
    GameObjectHeader gameObjectHeader; \
    __VA_OPT__(alignas(max_align_t) __VA_ARGS__ data;) \
} name

typedef struct GameObjectHandle
{
    uint32_t tag;
    uint32_t idx;
} GameObjectHandle;

typedef struct GameObjectCallbacks
{
    void(*init)  (GameObjectHandle, void*);
    void(*update)(GameObjectHandle);
    void(*draw)  (GameObjectHandle);
    void(*deinit)(GameObjectHandle);
} GameObjectCallbacks;

typedef struct GameObjectHeader
{
    const uint64_t id;
    Vector2 pos;
    const uint32_t index;
    const uint32_t tag;
    bool destroyed;
} GameObjectHeader;

typedef struct GameObject
{
    GameObjectHeader gameObjectHeader;
    alignas(max_align_t) unsigned char anyData[];
} GameObject;

GameObject *GetGameObject(GameObjectHandle handle);
GameObjectHandle GetHandle(GameObject *go);
void PushGameObjectGroup(uint32_t tag, GameObjectCallbacks callbacks, uint64_t gameObjSize, const char *typeName);
GameObject *AllocGameObject(uint32_t tag);
void DrawAllGameObjects();
void UpdateAllGameObjects();
uint32_t GameObjectTag(GameObjectHandle handle);
void DestroyGameObject(GameObject *handle);

GameObject *CreateGameObject_ByTag(uint32_t tag, void *arg);
GameObject *CreateGameObject_ByName(const char *name, void *arg);

uint32_t NameToTag(const char *typeName);
const char *TagToName(uint32_t tag);

void SeedRNG(uint32_t seed);
uint32_t RandomUInt();
float RandomFloat01();
float RandomFloat(float min, float max);

typedef struct Circle
{
    Vector2 center;
    float radius;
} Circle;

bool CirclesOverlapping(Circle a, Circle b);

static inline Vector2 *Position(GameObject *go)
{
    return &go->gameObjectHeader.pos;
}

#define CreateGameObject(tagOrName, arg)       \
_Generic(tagOrName,                                  \
char*               : CreateGameObject_ByName, \
unsigned char*      : CreateGameObject_ByName, \
signed char*        : CreateGameObject_ByName, \
default             : CreateGameObject_ByTag   \
)(tagOrName, arg)

extern uint32_t nextGameObjectTag;
extern uint64_t nextId;
// extern GameState gameState;

extern int windowWidth;
extern int windowHeight;

#endif
