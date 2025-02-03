#ifndef GAME_H
#define GAME_H

#include <malloc.h>
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
    bool destroy;
} GameObjectHeader;

typedef struct GameObject
{
    GameObjectHeader gameObjectHeader;
    alignas(max_align_t) unsigned char anyData[];
} GameObject;

DECL_GAMEOBJECT(
    Player,
    struct PlayerData
    {
        Vector2 velocity;
        float radius;
        float speed;
        int8_t health;
    }
);

DECL_GAMEOBJECT(
    Bullet,
    struct BulletData
    {
        Vector2 spawnPos;
        Vector2 target;
        Vector2 dir;
        float radius;
        float speed;
        Color color;
    }
);

DECL_GAMEOBJECT(
    Enemy,
    struct EnemyData
    {
        Vector2 spawnPos;
        float fireRate;
        float burstDelay;
        float radius;
        Color color;
        struct BulletData bulletData;
    }
);

DECL_GAMEOBJECT(
    EnemySpawner,
    struct EnemySpawnerData
    {
        float spawnSpeed;
        struct EnemyData nextEnemy;
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
void DestroyGameObject(GameObject *go);

GameObject *CreateGameObject_ByTag(uint32_t tag, void *arg);
GameObject *CreateGameObject_ByName(const char *name, void *arg);

uint32_t NameToTag(const char *typeName);
const char *TagToName(uint32_t tag);

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
extern GameState gameState;

extern int windowWidth;
extern int windowHeight;

#endif
