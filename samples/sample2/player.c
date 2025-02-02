#include "raylib.h"
#include "raymath.h"
#include "game.h"

#define GAMEOBJECT_TYPE Player
#include "gameobject.h"

void HandlePlayerInput(Player *p);
void HandlePlayerMovement(Player *p);

static void Init(GameObject *go)
{
    Player *player = (Player*)go;
    player->data.radius = 15.5f;
    player->data.speed = 355.0f;
    player->gameObjectHeader.pos = (Vector2){windowWidth / 2.0f, windowHeight / 2.0f};
}

static void Update(GameObject *go)
{
    HandlePlayerInput((Player*)go);
    HandlePlayerMovement((Player*)go);
}

static void Draw(GameObject *go)
{
    Player *player = (Player*)go;
    DrawCircleV(player->gameObjectHeader.pos, player->data.radius, PURPLE);
}

void HandlePlayerInput(Player *p)
{
    float deltaTime = GetFrameTime();
    if(IsKeyDown(KEY_W))
    {
        p->data.velocity.y += -p->data.speed * GetFrameTime();
    }
    if(IsKeyDown(KEY_A))
    {
        p->data.velocity.x += -p->data.speed * GetFrameTime();
    }
    if(IsKeyDown(KEY_S))
    {
        p->data.velocity.y += p->data.speed * GetFrameTime();
    }
    if(IsKeyDown(KEY_D))
    {
        p->data.velocity.x += p->data.speed * GetFrameTime();
    }
}

void HandlePlayerMovement(Player *p)
{
    bool outRight = p->gameObjectHeader.pos.x + p->data.radius >= windowWidth;
    bool outLeft = p->gameObjectHeader.pos.x - p->data.radius <= 0;
    if(outRight || outLeft)
    {
        float mult = (float[]){1.f,-1.f}[outRight];
        p->gameObjectHeader.pos.x = (outRight * windowWidth) + (p->data.radius * mult);
        p->data.velocity.x /= 2;
        p->data.velocity.x *= -1;
    }
    bool outDown = p->gameObjectHeader.pos.y + p->data.radius >= windowHeight;
    bool outUp = p->gameObjectHeader.pos.y - p->data.radius <= 0;
    if(outDown || outUp)
    {
        float mult = (float[]){1.f,-1.f}[outDown];
        p->gameObjectHeader.pos.y = (outDown * windowHeight) + (p->data.radius * mult);
        p->data.velocity.y /= 2;
        p->data.velocity.y *= -1;
    }
    
    p->gameObjectHeader.pos.x += p->data.velocity.x * GetFrameTime();
    p->gameObjectHeader.pos.y += p->data.velocity.y * GetFrameTime();
    
    p->data.velocity.x = Lerp(p->data.velocity.x, 0, 0.5f * GetFrameTime());
    p->data.velocity.y = Lerp(p->data.velocity.y, 0, 0.5f * GetFrameTime());
}
