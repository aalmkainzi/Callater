#if defined(GAMEOBJECT_DEFINED_IN_FILE)
#error "Only one gameobject type can be defined per file"
#endif

#define GAMEOBJECT_DEFINED_IN_FILE

#if !defined(GAMEOBJECT_TYPE)
#error "GAMEOBJECT_TYPE must be defined (e.g. `#define GAMEOBJECT_TYPE Enemy`)"
#endif

#include "game.h"

#define GAMEOBJ_TYPE_STR_(ty) #ty
#define GAMEOBJ_TYPE2STR(ty) GAMEOBJ_TYPE_STR_(ty)

static uint32_t gameObjTag;

static void Init(GameObjectHandle, void*);
static void Update(GameObjectHandle);
static void Draw(GameObjectHandle);
static void Deinit(GameObjectHandle);

static inline uint32_t ThisTag()
{
    return gameObjTag;
}

__attribute__((constructor)) static void RegisterGameObjectGroup()
{
    gameObjTag = nextGameObjectTag++;
    GameObjectCallbacks callbacks = {.init = Init, .update = Update, .draw = Draw, .deinit = Deinit};
    PushGameObjectGroup(ThisTag(), callbacks, sizeof(GAMEOBJECT_TYPE), GAMEOBJ_TYPE2STR(GAMEOBJECT_TYPE));
}
