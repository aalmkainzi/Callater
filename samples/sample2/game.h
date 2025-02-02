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
    \
    alignas(max_align_t) dataField data; \
} name;

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
    
    GameObjectCallbacks clalbacks;
    
    uint32_t tag;
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
)

DECL_GAMEOBJECT(
    Enemy,
    struct
    {
        Vector2 pos;
        float fireRate;
        float burstDelay;
    }
)

DECL_GAMEOBJECT(
    Bullet,
    struct
    {
        float speed;
    }
)

typedef struct GameObjectGroup
{
    uint64_t count, cap, elmSize;
    GameObject *objs;
} GameObjectGroup;

typedef struct GameState
{
    GameObjectGroup *gameObjectGroups;
    uint64_t cap, len;
    
    Vector2 bounds;
} GameState;

extern uint32_t nextGameObjectTag;
extern uint64_t nextId;
extern GameState gameState;

#endif