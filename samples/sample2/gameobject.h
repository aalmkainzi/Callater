#if defined(GAMEOBJECT_DEFINED_IN_FILE)
#error "Only one gameObject type can be defined per file"
#endif

#if !defined(GAMEOBJECT_TYPE)
#error "GAMEOBJECT_TYPE must be defined (e.g. `#define GAMEOBJECT_TYPE Enemy`)"
#endif

#define GAMEOBJECT_DEFINED_IN_FILE

#include "game.h"

#define GAMEOBJ_TYPE_STR_(ty) #ty
#define GAMEOBJ_TYPE2STR(ty) GAMEOBJ_TYPE_STR_(ty)

static uint32_t gameObjTyTag;

static void Init(GameObject *go);
static void Update(GameObject *go);
static void Draw(GameObject *go);

static uint32_t GetTyTag()
{
    return gameObjTyTag;
}

__attribute__((constructor)) static void RegisterGameObjectGroup()
{
    gameObjTyTag = nextGameObjectTag++;
    PushGameObjectGroup(GetTyTag(), (GameObjectCallbacks){.init = Init, .update = Update, .draw = Draw}, sizeof(GAMEOBJECT_TYPE), GAMEOBJ_TYPE2STR(GAMEOBJECT_TYPE));
}
