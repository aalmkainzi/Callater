#include <stdio.h>
#include "game.h"
#include "callater.h"
#include "raylib.h"
#include "gameobjects/enemy.h"
#include "gameobjects/player.h"

#define GAMEOBJECT_TYPE Enemy
#include "gameobject.h"

static uint32_t bulletTag = (uint32_t)-1;

FILE *outfile = NULL;

void ShootAtPlayer(void *arg, CallaterRef invokeRef)
{
    (void) invokeRef;
    
    Enemy *enemy = (Enemy*) GetGameObject(TYPE_PUN(arg, GameObjectHandle));
    struct BulletData bulletData = enemy->data.bulletData;
    bulletData.spawnPos = enemy->gameObjectHeader.pos;
    CreateGameObject(bulletTag, &bulletData);
}

static void Init(GameObjectHandle handle, void *arg)
{
    Enemy *enemy = (Enemy*) GetGameObject(handle);
    struct EnemyData *ed = arg;
    enemy->data = *ed;
    bulletTag = bulletTag == (uint32_t)-1 ? NameToTag("Bullet") : bulletTag;
    enemy->gameObjectHeader.pos = enemy->data.spawnPos;
    
    if(outfile == NULL) outfile = fopen("OUTFILE.txt", "w");
    fprintf(outfile, "FIRERATE: %g\n", enemy->data.fireRate);
    InvokeRepeatGID(ShootAtPlayer, TYPE_PUN(handle, void*), enemy->data.fireRate, enemy->data.fireRate, enemy->gameObjectHeader.id);
}

static void Update(GameObjectHandle handle)
{
    Enemy *enemy = (Enemy*) GetGameObject(handle);
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
        PlayerHeal(1);
        DestroyGameObject((GameObject*) enemy);
    }
}

static void Draw(GameObjectHandle handle)
{
    Enemy *enemy = (Enemy*) GetGameObject(handle);
    DrawCircleV(enemy->gameObjectHeader.pos, enemy->data.radius, enemy->data.color);
}

static void Deinit(GameObjectHandle handle)
{
    (void) handle;
}
