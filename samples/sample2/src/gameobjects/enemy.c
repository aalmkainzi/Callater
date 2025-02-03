#include "game.h"
#include "player.h"
#include "callater.h"
#include "raylib.h"
#include "raymath.h"

#define GAMEOBJECT_TYPE Enemy
#include "gameobject.h"

static uint32_t bulletTag = (uint32_t)-1;

void ShootAtPlayer(void *arg, CallaterRef invokeRef)
{
    (void) invokeRef;
    
    Enemy *enemy = arg;
    struct BulletData bulletData = enemy->data.bulletData;
    bulletData.spawnPos = enemy->gameObjectHeader.pos;
    CreateGameObject(bulletTag, &bulletData);
}

static void Init(GameObject *go, void *arg)
{
    Enemy *enemy = (Enemy*) go;
    struct EnemyData *ed = arg;
    enemy->data = *ed;
    bulletTag = bulletTag == (uint32_t)-1 ? NameToTag("Bullet") : bulletTag;
    enemy->gameObjectHeader.pos = enemy->data.spawnPos;
    
    InvokeRepeatGID(ShootAtPlayer, enemy, enemy->data.fireRate, enemy->data.fireRate, enemy->gameObjectHeader.id);
}

static void Update(GameObject *go)
{
    Enemy *enemy = (Enemy*) go;
    Player *player = (Player*) playerInstance;
    
    Circle enemyCircle = {
        .center = enemy->gameObjectHeader.pos,
        .radius = enemy->data.radius
    };
    Circle playerCircle = {
        .center = player->gameObjectHeader.pos,
        .radius = player->data.radius
    };
    
    if( CirclesOverlapping(enemyCircle, playerCircle) )
    {
        DestroyGameObject((GameObject*) enemy);
    }
}

static void Draw(GameObject *go)
{
    Enemy *enemy = (Enemy*) go;
    DrawCircleV(enemy->gameObjectHeader.pos, enemy->data.radius, enemy->data.color);
}
