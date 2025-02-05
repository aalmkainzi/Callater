#include "game.h"
#include "raylib.h"
#include "callater.h"
#include "gameobjects/enemy_spawner.h"

#define GAMEOBJECT_TYPE EnemySpawner
#include "gameobject.h"

static uint32_t enemyTag = (uint32_t)-1;

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
        .speed = RandomFloat(35.0f, 65.5f)
    };
    return ret;
}

void SteadySpawning(void *arg, CallaterRef invokeRef)
{
    (void) invokeRef;
    
    EnemySpawner *spawner = (EnemySpawner*) GetGameObject(TYPE_PUN(arg, GameObjectHandle));
    spawner->data.nextEnemy = RandomEnemyData();
    CreateGameObject(enemyTag, &spawner->data.nextEnemy);
}

void SpeedUpSpawning(void *arg, CallaterRef invokeRef)
{
    EnemySpawner *spawner = (EnemySpawner*) GetGameObject(TYPE_PUN(arg, GameObjectHandle));
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

static void Init(GameObjectHandle handle, void *arg)
{
    (void) arg;
    
    EnemySpawner *spawner = (EnemySpawner*) GetGameObject(handle);
    enemyTag = (enemyTag == (uint32_t)-1 ? NameToTag("Enemy") : enemyTag);
    spawner->data.spawnSpeed = 5.0f;
    InvokeRepeatGID(SpeedUpSpawning, TYPE_PUN(handle, void*), spawner->data.spawnSpeed, spawner->data.spawnSpeed, spawner->gameObjectHeader.id);
}

static void Update(GameObjectHandle handle)
{
    (void) handle;
}

static void Draw(GameObjectHandle handle)
{
    (void) handle;
}

static void Deinit(GameObjectHandle handle)
{
    (void) handle;
}
