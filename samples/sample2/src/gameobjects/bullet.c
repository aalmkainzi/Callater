#include "raylib.h"
#include "raymath.h"
#include "game.h"
#include "gameobjects/player.h"
#include "gameobjects/bullet.h"

#define GAMEOBJECT_TYPE Bullet
#include "gameobject.h"

static void Init(GameObjectHandle handle, void *arg)
{
    Bullet *bullet = (Bullet*) GetGameObject(handle);
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

static void Update(GameObjectHandle handle)
{
    Bullet *bullet = (Bullet*) GetGameObject(handle);
    
    bullet->gameObjectHeader.pos =
    Vector2Add(bullet->gameObjectHeader.pos, 
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
        return;
    }
    if( 
        (bulletCircle.center.x - bulletCircle.radius > windowWidth)   ||
        (bulletCircle.center.x + bulletCircle.radius <  0)            ||
        (bulletCircle.center.y - bulletCircle.radius >  windowHeight) ||
        (bulletCircle.center.y + bulletCircle.radius < 0)
    )
    {
        DestroyGameObject((GameObject*) bullet);
        return;
    }
}

static void Draw(GameObjectHandle handle)
{
    Bullet *bullet = (Bullet*) GetGameObject(handle);
    DrawCircleV(bullet->gameObjectHeader.pos, bullet->data.radius, RED);
}

static void Deinit(GameObjectHandle handle)
{
    (void) handle;
}
