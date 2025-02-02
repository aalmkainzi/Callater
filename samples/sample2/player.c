#include "game.h"
#include "raylib.h"

void DrawPlayer(void *p)
{
    Player *player = p;
    DrawCircleV(player->gameObject.pos, player->radius, PURPLE);
}

void UpdatePlayer()
{
    
}

void HandlePlayerInput(Player *p)
{
    float deltaTime = GetFrameTime();
    if(IsKeyDown(KEY_W))
    {
        p->velocity.y += -p->speed * GetFrameTime();
    }
    if(IsKeyDown(KEY_A))
    {
        p->velocity.x += -p->speed * GetFrameTime();
    }
    if(IsKeyDown(KEY_S))
    {
        p->velocity.y += p->speed * GetFrameTime();
    }
    if(IsKeyDown(KEY_D))
    {
        p->velocity.x += p->speed * GetFrameTime();
    }
}

void HandlePlayerMovement(Player *p)
{
    bool outRight = p->pos.x + playerRadius >= windowWidth;
    bool outLeft = p->pos.x - playerRadius <= 0;
    if(outRight || outLeft)
    {
        float mult = (float[]){1.f,-1.f}[outRight];
        p->pos.x = (outRight * windowWidth) + (playerRadius * mult);
        p->velocity.x /= 2;
        p->velocity.x *= -1;
    }
    bool outDown = p->pos.y + playerRadius >= windowHeight;
    bool outUp = p->pos.y - playerRadius <= 0;
    if(outDown || outUp)
    {
        float mult = (float[]){1.f,-1.f}[outDown];
        p->pos.y = (outDown * windowHeight) + (playerRadius * mult);
        p->velocity.y /= 2;
        p->velocity.y *= -1;
    }
    
    p->pos.x += p->velocity.x * GetFrameTime();
    p->pos.y += p->velocity.y * GetFrameTime();
    
    p->velocity.x = Lerp(p->velocity.x, 0, 0.5f * GetFrameTime());
    p->velocity.y = Lerp(p->velocity.y, 0, 0.5f * GetFrameTime());
}