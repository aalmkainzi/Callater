#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../../callater.h"


#define FOODSIZE (3.5f)
typedef struct PlayerCircle
{
    Vector2 pos;
    float radius;
} PlayerCircle;

typedef struct FoodData
{
    Vector2 bounds;
    Vector2 foods[1000];
    int nbFoods;
} FoodData;

void DrawPlayer(PlayerCircle *p);
void HandlePlayerMovement(PlayerCircle *p, Camera2D *cam);
void HandleFoodEating(PlayerCircle *p, FoodData *foodData);
void SpawnRandomFood(void *bounds);

int main()
{
    InitWindow(1280, 720, "Meow");
    srand(time(NULL));
    
    CallaterInit();
    SetTargetFPS(60);
    
    Camera2D cam = {0};
    cam.zoom = 1.0f;
    
    InitAudioDevice();
    Sound pop = LoadSound("resources/pop.wav");
    
    if(!IsSoundValid(pop))
    {
        puts("err");
        exit(1);
    }
    
    FoodData foodData =
    {
        .foods = {
            {15,15},
            {100,100},
            {64,64}
        },
        .nbFoods = 3,
        .bounds = {GetScreenWidth(), GetScreenHeight()}
    };
    
    PlayerCircle player = {
        .pos = {500, 500},
        .radius = 16
    };
    
    InvokeRepeat(SpawnRandomFood, &foodData, 1.0f, 1.0f);
    
    while(!WindowShouldClose())
    {
        BeginDrawing();
        BeginMode2D(cam);
        
            ClearBackground(DARKGRAY);
            for(int i = 0 ; i < foodData.nbFoods ; i++)
            {
                DrawCircleV(foodData.foods[i], FOODSIZE, GREEN);
            }
            DrawPlayer(&player);
            
            HandlePlayerMovement(&player, &cam);
            HandleFoodEating(&player, &foodData);
        
        CallaterUpdate();
        EndMode2D();
        EndDrawing();
    }
}

void DrawPlayer(PlayerCircle *player)
{
    DrawCircleV(player->pos, player->radius, RED);
}

void HandlePlayerMovement(PlayerCircle *p, Camera2D *cam)
{
    Vector2 mousePos = GetMousePosition();
    Vector2 mouseWorld = GetScreenToWorld2D(mousePos, *cam);
    float speed = Vector2Distance(mouseWorld, p->pos) / 16.0f;
    speed = Clamp(speed, 0, 3.0f);
    p->pos = Vector2MoveTowards(p->pos, mouseWorld, speed);
}

void HandleFoodEating(PlayerCircle *p, FoodData *foodData)
{
    for(int i = 0 ; i < foodData->nbFoods ; i++)
    {
        if(Vector2Distance(p->pos, foodData->foods[i]) <= p->radius)
        {
            p->radius += 2.15f;
            foodData->foods[i] = foodData->foods[foodData->nbFoods - 1];
            foodData->nbFoods--;
            i--;
        }
    }
}

void SpawnRandomFood(void *foodData_)
{
    FoodData *foodData = foodData_;
    if(foodData->nbFoods >= (sizeof(foodData->foods) / sizeof(foodData->foods[0])) - 1)
    {
        return;
    }
    
    float xPercent = 1.0f * rand() / RAND_MAX;
    float yPercent = 1.0f * rand() / RAND_MAX;
    Vector2 newFoodPos = {
        .x = xPercent * foodData->bounds.x,
        .y = yPercent * foodData->bounds.y
    };
    foodData->foods[foodData->nbFoods++] = newFoodPos;
}
