#include <stdlib.h>
#include <stdio.h>
#include "raylib.h"
#include "raymath.h"
#include "callater.h"
#include "game.h"
#include "samples/sample1/raylib-5.5_win64_mingw-w64/include/raylib.h"
#include "gameobjects/player.h"

#define GAMEOBJECT_TYPE Player
#include "gameobject.h"

Player *playerInstance = {0};

static void HandlePlayerInput(Player *p);
static void HandlePlayerMovement(Player *p);

static void Init(GameObjectHandle handle, void *arg)
{
    (void) arg;
    
    Player *player = (Player*) GetGameObject(handle);
    playerInstance = player;
    player->data.radius = 15.5f;
    player->data.speed = 355.0f;
    player->data.health = 5;
    player->data.textColor = WHITE;
    player->data.colorChangeInvokeRef = CALLATER_REF_ERR;
    player->gameObjectHeader.pos = (Vector2){windowWidth / 2.0f, windowHeight / 2.0f};
}

static void Update(GameObjectHandle handle)
{
    GameObject *go = GetGameObject(handle);
    HandlePlayerInput((Player*)go);
    HandlePlayerMovement((Player*)go);
}

static void Draw(GameObjectHandle handle)
{
    Player *player = (Player*) GetGameObject(handle);
    DrawCircleV(player->gameObjectHeader.pos, player->data.radius, PURPLE);
    char buf[10] = {0};
    snprintf(buf, sizeof(buf), "%hhd", player->data.health);
    DrawText(buf, 10, 10, 55, player->data.textColor);
}

static void Deinit(GameObjectHandle handle)
{
    (void) handle;
}

void HandlePlayerInput(Player *p)
{
    float deltaTime = GetFrameTime();
    if(IsKeyDown(KEY_W))
    {
        p->data.velocity.y += -p->data.speed * deltaTime;
    }
    if(IsKeyDown(KEY_A))
    {
        p->data.velocity.x += -p->data.speed * deltaTime;
    }
    if(IsKeyDown(KEY_S))
    {
        p->data.velocity.y += p->data.speed * deltaTime;
    }
    if(IsKeyDown(KEY_D))
    {
        p->data.velocity.x += p->data.speed * deltaTime;
    }
}

void HandlePlayerMovement(Player *p)
{
    float deltaTime = GetFrameTime();
    
    static const float sign[] = {1.f,-1.f};
    bool outRight = p->gameObjectHeader.pos.x + p->data.radius > windowWidth;
    bool outLeft  = p->gameObjectHeader.pos.x - p->data.radius < 0;
    if(outRight || outLeft)
    {
        float mult = sign[outRight];
        p->gameObjectHeader.pos.x = (outRight * windowWidth) + (p->data.radius * mult);
        p->data.velocity.x /= 2;
        p->data.velocity.x *= -1;
    }
    
    bool outDown = p->gameObjectHeader.pos.y + p->data.radius > windowHeight;
    bool outUp   = p->gameObjectHeader.pos.y - p->data.radius < 0;
    if(outDown || outUp)
    {
        float mult = sign[outDown];
        p->gameObjectHeader.pos.y = (outDown * windowHeight) + (p->data.radius * mult);
        p->data.velocity.y /= 2;
        p->data.velocity.y *= -1;
    }
    
    p->gameObjectHeader.pos.x += p->data.velocity.x * deltaTime;
    p->gameObjectHeader.pos.y += p->data.velocity.y * deltaTime;
    
    p->data.velocity.x = Lerp(p->data.velocity.x, 0, 0.5f * deltaTime);
    p->data.velocity.y = Lerp(p->data.velocity.y, 0, 0.5f * deltaTime);
}

void ResetTextColor(void *arg, CallaterRef invokeRef)
{
    Player *player = (Player*) GetGameObject(TYPE_PUN(arg, GameObjectHandle)); // can just use playerInstance here...
    
    player->data.colorChangeInvokeRef = CALLATER_REF_ERR;
    player->data.textColor = WHITE;
}

void PlayerTakeDamage(uint8_t dmg)
{
    Player *player = (Player*) playerInstance;
    player->data.health -= dmg;
    player->data.textColor = RED;
    
    if(player->data.health <= 0)
    {
        exit(0);
    }
    
    if( !CallaterRefError(player->data.colorChangeInvokeRef) )
    {
        CallaterCancel(player->data.colorChangeInvokeRef);
    }
    InvokeGID(ResetTextColor, TYPE_PUN(GetHandle((GameObject*) player), void*), 0.2f, player->gameObjectHeader.id);
}

void PlayerHeal(uint8_t healBy)
{
    Player *player = (Player*) playerInstance;
    player->data.health += healBy;
    player->data.textColor = GREEN;
    
    if( !CallaterRefError(player->data.colorChangeInvokeRef) )
    {
        CallaterCancel(player->data.colorChangeInvokeRef);
    }
    InvokeGID(ResetTextColor, TYPE_PUN(GetHandle((GameObject*) player), void*), 0.2f, player->gameObjectHeader.id);
}
