#include <stdlib.h>
#include "game.h"
#include "../../callater.h"

#define GAMEOBJECT_TYPE EnemySpawner
#include "gameobject.h"

void SteadySpawning(void *arg, CallaterRef invokeRef)
{
    EnemySpawner *spawner = arg;
    CreateGameObject(spawner->data.enemyTag, NULL);
}

void SpeedUpSpawning(void *arg, CallaterRef invokeRef)
{
    EnemySpawner *spawner = arg;
    CreateGameObject(spawner->data.enemyTag, NULL);
    
    spawner->data.spawnSpeed -= 0.2f;
    if(spawner->data.spawnSpeed <= 1.0f)
    {
        CallaterSetFunc(invokeRef, SteadySpawning);
    }
    else
    {
        CallaterSetRepeatRate(invokeRef, spawner->data.spawnSpeed);
    }
}

static void Init(GameObject *go, void *arg)
{
    EnemySpawner *spawner = (EnemySpawner*) go;
    spawner->data.enemyTag = NameToTag("Enemy");
    spawner->data.spawnSpeed = 5.0f;
    InvokeRepeatGID(SpeedUpSpawning, go, spawner->data.spawnSpeed, spawner->data.spawnSpeed, spawner->gameObjectHeader.id);
}

static void Update(GameObject *go)
{
    ;
}

static void Draw(GameObject *go)
{
    ;
}
