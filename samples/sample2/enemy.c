#include "game.h"
#include "player.h"
#include "../../callater.h"

#define GAMEOBJECT_TYPE Enemy
#include "gameobject.h"

void ShootAtPlayer(void *arg, CallaterRef invokeRef)
{
    Enemy *enemy = arg;
    
}

static void Init(GameObject *go, void *arg)
{
    Enemy *enemy = (Enemy*) go;
}

static void Update(GameObject *go)
{
    
}

static void Draw(GameObject *go)
{
    
}
