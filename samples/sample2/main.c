#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../../callater.h"
#include "game.h"

GameState gameState = { 0 };
uint32_t nextGameObjectTag = 0;

int windowWidth = 1280;
int windowHeight = 720;

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

void DrawAll()
{
    for(uint64_t i = 0 ; i < gameState.nbDrawables ; i++)
    {
        gameState.drawables[i].draw(gameState.drawables[i].arg);
    }
}

void NoopDraw(void *arg)
{
    (void)arg;
}

void RemoveDrawable(uint64_t idx)
{
    gameState.drawables[idx] = (Drawable){.draw = NoopDraw, .arg = NULL};
}

// for now this just pushes to the back.
// I should look for empty spots in the array to populate instead
uint64_t AddDrawable(Drawable drawable)
{
    if(gameState.nbDrawables >= gameState.capDrawables)
    {
        gameState.capDrawables *= 2;
        gameState.drawables = realloc(gameState.drawables, gameState.capDrawables);
    }
    
    gameState.drawables[gameState.nbDrawables] = drawable;
    gameState.nbDrawables += 1;
    return gameState.nbDrawables - 1;
}
