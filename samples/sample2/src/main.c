#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "callater.h"
#include "game.h"

int windowWidth = 1280;
int windowHeight = 720;

Scene defaultScene;

int main()
{
    CallaterInit();
    InitWindow(windowWidth, windowHeight, "Bam");
    
    // SeedRNG(time(NULL));
    
    uint32_t playerTag    = NameToTag("Player");
    uint32_t spawnerTag   = NameToTag("EnemySpawner");
    uint32_t inspectorTag = NameToTag("Inspector");
    
    defaultScene = (Scene){
        .tags  = (uint32_t[]){playerTag, spawnerTag, inspectorTag},
        .args  = (void*[])   {NULL     , NULL      , NULL},
        .count = 3
    };
    
    GameLoop(defaultScene);
}
