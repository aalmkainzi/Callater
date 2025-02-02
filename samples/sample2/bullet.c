#include "game.h"
#include "player.h"
#define GAMEOBJECT_TYPE Bullet
#include "gameobject.h"

static void Init(GameObject *go, void *posAndSpeed)
{
    Bullet *bullet = (Bullet*) go;
    
    struct {
        Vector2 spawnPos;
        float speed;
    } bulletData = *(typeof(bulletData)*) posAndSpeed;
    
    bullet->gameObjectHeader.pos = bulletData.spawnPos;
    bullet->data.speed = bulletData.speed;
    bullet->data.target = playerInstance->gameObjectHeader.pos;
}

static void Update(GameObject *go)
{
    Bullet *bullet = (Bullet*) go;
}

static void Draw(GameObject *go)
{
    Bullet *bullet = (Bullet*) go;
}
