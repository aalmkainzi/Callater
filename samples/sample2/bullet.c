#include "game.h"
#include "player.h"
#include "raylib.h"
#include "raymath.h"

#define GAMEOBJECT_TYPE Bullet
#include "gameobject.h"

static void Init(GameObject *go, void *arg)
{
    Bullet *bullet = (Bullet*) go;
    bullet->data = *(struct BulletData*)arg;
    
    bullet->data.target = playerInstance->gameObjectHeader.pos;
    bullet->gameObjectHeader.pos = bullet->data.spawnPos;
    bullet->data.dir = Vector2Subtract(bullet->data.target, bullet->gameObjectHeader.pos);
    bullet->data.dir = Vector2Normalize(bullet->data.dir);
}

static Vector2 Vector2MultFloat(Vector2 v, float f)
{
    return (Vector2){
        .x = v.x * f,
        .y = v.y * f
    };
}

static void Update(GameObject *go)
{
    Bullet *bullet = (Bullet*) go;
    
    bullet->gameObjectHeader.pos = Vector2Add(bullet->gameObjectHeader.pos, 
                                              Vector2MultFloat(
                                                  Vector2MultFloat(bullet->data.dir, GetFrameTime()),
                                                  bullet->data.speed
                                              )
                                   );
    
    Player *player = (Player*) playerInstance;
    
    Circle bulletCircle = {
        .center = bullet->gameObjectHeader.pos,
        .radius = bullet->data.radius
    };
    Circle playerCircle = {
        .center = player->gameObjectHeader.pos,
        .radius = player->data.radius
    };
    
    if( CirclesOverlapping(bulletCircle, playerCircle) )
    {
        PlayerTakeDamage(1);
        DestroyGameObject((GameObject*) bullet);
    }
}

static void Draw(GameObject *go)
{
    Bullet *bullet = (Bullet*) go;
    DrawCircleV(bullet->gameObjectHeader.pos, bullet->data.radius, RED);
}
