#if defined(GAMEOBJECT_DEFINED_IN_FILE)
#error "Only one gameObject type can be defined per file"
#endif

#if !defined(GAMEOBJECT_TYPE)
#error "GAMEOBJECT_TYPE must be defined (e.g. `#define GAMEOBJECT_TYPE Enemy`)"
#endif

#define GAMEOBJECT_DEFINED_IN_FILE

#include "game.h"

static uint32_t gameObjTyTag;

__attribute__((constructor)) static void RegisterGameObjectTag()
{
    gameObjTyTag = nextGameObjectTag++;
}

static uint32_t GetTyTag()
{
    return gameObjTyTag;
}

static void Init(GameObject *go);
static void Update(GameObject *go);
static void Draw(GameObject *go);

static GameObject *CreateGameObject()
{
    GAMEOBJECT_TYPE ret = {
        .gameObjectHeader.id = nextId++,
        .gameObjectHeader.tag = GetTyTag(),
        .gameObjectHeader.callbacks = {.init = Init, .update = Update, .draw = Draw}
    };
    
    ret.gameObjectHeader.Init(&ret);
    return ret;
}
