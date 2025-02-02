#if defined(GAMEOBJECT_DEFINED_IN_FILE)
#error "Only one gameObject type can be defined per file"
#endif

#define GAMEOBJECT_DEFINED_IN_FILE

#if !defined(GAMEOBJECT_TYPE)
#error "GAMEOBJECT_TYPE must be defined (e.g. `#define GAMEOBJECT_TYPE Enemy`)"
#endif

#include "game.h"

#define GAMEOBJ_TYPE_STR_(ty) #ty
#define GAMEOBJ_TYPE2STR(ty) GAMEOBJ_TYPE_STR_(ty)

static uint32_t gameObjTyTag;

static void Init(GameObject*, void*);
static void Update(GameObject*);
static void Draw(GameObject*);

static inline uint32_t ThisTag()
{
    return gameObjTyTag;
}

__attribute__((constructor)) static void RegisterGameObjectGroup()
{
    gameObjTyTag = nextGameObjectTag++;
    PushGameObjectGroup(ThisTag(), (GameObjectCallbacks){.init = Init, .update = Update, .draw = Draw}, sizeof(GAMEOBJECT_TYPE), GAMEOBJ_TYPE2STR(GAMEOBJECT_TYPE));
}
