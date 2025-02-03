#include "game.h"
#include "player.h"
#include "../../callater.h"
#include "raylib.h"
#include "raymath.h"

#define GAMEOBJECT_TYPE Enemy
#include "gameobject.h"

void ShootAtPlayer(void *arg, CallaterRef invokeRef)
{
    Enemy *enemy = arg;
    
}

static void Init(GameObject *go, void *arg)
{
    Enemy *enemy = (Enemy*) go;
    struct EnemyData *ed = arg;
    enemy->data = *ed;
}

static bool maxf(float a, float b)
{
    return a > b ? a : b;
}

typedef struct Circle
{
    Vector2 center;
    float radius;
} Circle;

static bool CirclesOverlapping(Circle a, Circle b)
{
    float dist = Vector2Distance(a.center, b.center);
    return dist <= maxf(a.radius, b.radius);
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
