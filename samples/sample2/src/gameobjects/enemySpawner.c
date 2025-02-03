#include <stdlib.h>
#include "game.h"
#include "callater.h"
#include "raylib.h"

#define GAMEOBJECT_TYPE EnemySpawner
#include "gameobject.h"

uint32_t enemyTag = (uint32_t)-1;

struct EnemyData RandomEnemyData()
{
    struct EnemyData ret = {0};
    ret.radius = RandomFloat(7.0f, 25.0f);
    ret.color = (Color){ GetRandomValue(0, 255), GetRandomValue(0, 255), GetRandomValue(0, 255), 255 };
    ret.fireRate = RandomFloat(0.75f, 5.0f);
    ret.spawnPos = (Vector2) {
        RandomFloat(ret.radius, windowWidth  - ret.radius),
        RandomFloat(ret.radius, windowHeight - ret.radius)
    };
    ret.bulletData = (struct BulletData){
        .color = RED,
        .radius = 5.0f,
        .speed = RandomFloat(35.0f, 65.5f),
    };
    return ret;
}

void SteadySpawning(void *arg, CallaterRef invokeRef)
{
    (void) invokeRef;
    
    EnemySpawner *spawner = arg;
    spawner->data.nextEnemy = RandomEnemyData();
    CreateGameObject(enemyTag, &spawner->data.nextEnemy);
}

void SpeedUpSpawning(void *arg, CallaterRef invokeRef)
{
    EnemySpawner *spawner = arg;
    spawner->data.nextEnemy = RandomEnemyData();
    CreateGameObject(enemyTag, &spawner->data.nextEnemy);
    
    spawner->data.spawnSpeed -= 0.2f;
    if(spawner->data.spawnSpeed <= 0.75f)
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
    (void) arg;
    
    EnemySpawner *spawner = (EnemySpawner*) go;
    enemyTag = (enemyTag == (uint32_t)-1 ? NameToTag("Enemy") : enemyTag);
    spawner->data.spawnSpeed = 5.0f;
    InvokeRepeatGID(SpeedUpSpawning, go, spawner->data.spawnSpeed, spawner->data.spawnSpeed, spawner->gameObjectHeader.id);
}

static void Update(GameObject *go)
{
    (void) go;
}

static void Draw(GameObject *go)
{
    (void) go;
}
