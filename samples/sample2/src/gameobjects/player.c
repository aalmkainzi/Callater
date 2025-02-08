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

static void Init(GameObject *go, void *arg)
{
    (void) arg;
    
    Player *player = (Player*)go;
    playerInstance = player;
    player->data.radius = 15.5f;
    player->data.speed = 355.0f;
    player->data.health = 5;
    player->data.textColor = WHITE;
    player->data.colorChangeInvokeRef = CALLATER_REF_ERR;
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
    char buf[10] = {0};
    snprintf(buf, sizeof(buf), "%hhd", player->data.health);
    DrawTextEx(GetFontDefault(), buf, (Vector2){10.0f,10.0f}, 64.f, 1.0f, player->data.textColor);
}

static void Deinit(GameObject *go)
{
    (void) go;
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

#define TYPE_PUN(x, to) \
((union { typeof(to) AsTo; typeof(x) AsOld; }){.AsOld = x}.AsTo)

void ResetTextColor(void *arg, CallaterRef invokeRef)
{
    Player *player = arg; // can just use playerInstance here...
    
    player->data.colorChangeInvokeRef = CALLATER_REF_ERR;
    player->data.textColor = WHITE;
}

void PlayerTakeDamage(uint8_t dmg)
{
    // return;
    extern Scene defaultScene;
    
    Player *player = (Player*) playerInstance;
    player->data.health -= dmg;
    player->data.textColor = RED;
    
    if(player->data.health <= 0)
    {
        UnloadScene();
        LoadScene(defaultScene);
    }
    
    if( !CallaterRefError(player->data.colorChangeInvokeRef) )
    {
        CallaterCancel(player->data.colorChangeInvokeRef);
    }
    
    InvokeGID(ResetTextColor, player, 1.5f, player->gameObjectHeader.id);
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
    
    InvokeGID(ResetTextColor, player, 1.5f, player->gameObjectHeader.id);
}
