#ifndef GAME_H
#define GAME_H

#include <stddef.h>
#include <stdint.h>
#include <stdalign.h>
#include "raylib.h"

#define DECL_GAMEOBJECT(name, dataField) \
typedef struct name \
{ \
    GameObjectHeader gameObjectHeader; \
    alignas(max_align_t) dataField data; \
} name

struct GameObject;

typedef struct GameObjectCallbacks
{
    void(*init)  (struct GameObject*);
    void(*update)(struct GameObject*);
    void(*draw)  (struct GameObject*);
} GameObjectCallbacks;

typedef struct GameObjectHeader
{
    uint64_t id;
    Vector2 pos;
} GameObjectHeader;

typedef struct GameObject
{
    GameObjectHeader gameObjectHeader;
    alignas(max_align_t) unsigned char gameObjectData[];
} GameObject;

DECL_GAMEOBJECT(
    Player,
    struct
    {
        Vector2 velocity;
        float radius;
        float speed;
        bool readyToFire;
    }
);

DECL_GAMEOBJECT(
    Enemy,
    struct
    {
        Vector2 pos;
        float fireRate;
        float burstDelay;
    }
);

DECL_GAMEOBJECT(
    Bullet,
    struct
    {
        float speed;
    }
);

DECL_GAMEOBJECT(
    EnemySpawner,
    struct
    {
        float spawnSpeed;
    }
);

typedef struct GameObjectGroup
{
    const char *name;
    uint64_t count, cap, elmSize;
    GameObject *objs;
    GameObjectCallbacks callbacks;
    uint32_t tag;
} GameObjectGroup;

typedef struct GameState
{
    GameObjectGroup *gameObjectGroups;
    uint64_t count, cap;
} GameState;

void PushGameObjectGroup(uint32_t tag, GameObjectCallbacks callbacks, uint64_t gameObjSize, const char *typeName);
GameObject *AllocGameObject(uint32_t tag);
void DrawAllGameObjects();
void UpdateAllGameObjects();

GameObject *CreateGameObject_ByTag(uint32_t tag);
GameObject *CreateGameObject_ByName(const char *name);

#define CreateGameObject(arg) \
_Generic(arg, \
char*: CreateGameObject_ByName, \
unsigned char*: CreateGameObject_ByName, \
signed char*: CreateGameObject_ByName, \
default: CreateGameObject_ByTag \
)(arg)

uint32_t NameToTag(const char *typeName);
const char *TagToName(uint32_t tag);



static inline Vector2 *Position(GameObject *go)
{
    return &go->gameObjectHeader.pos;
}

extern uint32_t nextGameObjectTag;
extern uint64_t nextId;
extern GameState gameState;

extern int windowWidth;
extern int windowHeight;

#endif
