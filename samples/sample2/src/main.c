#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "callater.h"
#include "game.h"

int windowWidth = 1280;
int windowHeight = 720;

int main()
{
    CallaterInit();
    InitWindow(windowWidth, windowHeight, "Bam");
    
    uint32_t playerTag = NameToTag("Player");
    CreateGameObject(playerTag, NULL);
    
    uint32_t spawnerTag = NameToTag("EnemySpawner");
    CreateGameObject(spawnerTag, NULL);
    while(!WindowShouldClose())
    {
        BeginDrawing();
            
            ClearBackground(BLACK);
            
            DrawAllGameObjects();
            UpdateAllGameObjects();
            
        CallaterUpdate();
        EndDrawing();
    }
}
