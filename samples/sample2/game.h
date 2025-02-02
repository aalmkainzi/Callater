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
    void(*init)  (struct GameObject*, void*);
    void(*update)(struct GameObject*);
    void(*draw)  (struct GameObject*);
} GameObjectCallbacks;

typedef struct GameObjectHeader
{
    const uint64_t id;
    Vector2 pos;
    const uint32_t tag;
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
        GameObject *player;
        Vector2 pos;
        float fireRate;
        float burstDelay;
        Color color;
    }
);

DECL_GAMEOBJECT(
    Bullet,
    struct
    {
        Vector2 target;
        float speed;
        float radius;
        Color color;
    }
);

DECL_GAMEOBJECT(
    EnemySpawner,
    struct
    {
        float spawnSpeed;
        uint32_t enemyTag;
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
uint32_t GameObjectTag(GameObject *go);

GameObject *CreateGameObject_ByTag(uint32_t tag, void *arg);
GameObject *CreateGameObject_ByName(const char *name, void *arg);

#define CreateGameObject(tagOrName, arg) \
_Generic(arg,                            \
char*         : CreateGameObject_ByName, \
unsigned char*: CreateGameObject_ByName, \
signed char*  : CreateGameObject_ByName, \
default       : CreateGameObject_ByTag   \
)(tagOrName, arg)

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
