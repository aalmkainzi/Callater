#include <stdlib.h>
#include "game.h"
#include "../../callater.h"
#include "raylib.h"

#define GAMEOBJECT_TYPE EnemySpawner
#include "gameobject.h"

void SteadySpawning(void *arg, CallaterRef invokeRef)
{
    EnemySpawner *spawner = arg;
    CreateGameObject(spawner->data.bulletTag, NULL);
}

void SpeedUpSpawning(void *arg, CallaterRef invokeRef)
{
    EnemySpawner *spawner = arg;
    CreateGameObject(spawner->data.bulletTag, NULL);
    
    float *spawnRate = (float*)arg;
    *spawnRate -= 0.2f;
    if(*spawnRate <= 0.5f)
    {
        CallaterSetFunc(invokeRef, SteadySpawning);
    }
    CallaterSetRepeatRate(invokeRef, *(float*)arg);
}

static void Init(GameObject *go, void *arg)
{
    EnemySpawner *spawner = (EnemySpawner*)go;
    spawner->data.bulletTag = NameToTag("Bullet");
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
