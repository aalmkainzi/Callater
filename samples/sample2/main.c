#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../../callater.h"
#include "game.h"

GameState gameState = {0};

int windowWidth = 1280;
int windowHeight = 720;

float playerRadius = 10.0f;

void DrawPlayer(void *p);
void HandlePlayerInput(Player *p);
void HandlePlayerMovement(Player *p);

int main()
{
    CallaterInit();
    InitWindow(windowWidth, windowHeight, "bam");
    
    Player player = {
        .drawPlayer = {.draw = DrawPlayer, .arg = &player},
        .pos = {500,500},
        .speed = 350.0f
    };
    
    gameState.player = &player;
    gameState.bounds = (Vector2){windowWidth, windowHeight};
    
    while(!WindowShouldClose())
    {
        BeginDrawing();
            
            ClearBackground(BLACK);
            HandlePlayerInput(&player);
            HandlePlayerMovement(&player);
            DrawPlayer(&player);
            
        CallaterUpdate();
        EndDrawing();
    }
}

void DrawPlayer(void *p)
{
    Player *player = p;
    DrawCircleV(player->pos, playerRadius, PURPLE);
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
