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
    Vector2 foods[1000];
    int nbFoods;
} FoodData;

void DrawPlayer(PlayerCircle *p);
void HandlePlayerMovement(PlayerCircle *p, Camera2D *cam);
void HandleFoodEating(PlayerCircle *p, FoodData *foodData);
void SpawnRandomFood(void *arg, CallaterRef invokeRef);

int windowWidth = 1280;
int windowHeight = 720;

Sound pop;

int main()
{
    InitWindow(1280, 720, "Meow");
    srand(time(NULL));
    
    CallaterInit();
    
    Camera2D cam = {0};
    cam.zoom = 1.0f;
    
    InitAudioDevice();
    pop = LoadSound("resources/pop.wav");
    
    FoodData foodData = {0};
    
    PlayerCircle player = {
        .pos = {500, 500},
        .radius = 16
    };
    
    InvokeRepeat(SpawnRandomFood, &foodData, 0, 1.0f);
    
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
    float speed = Vector2Distance(mouseWorld, p->pos);
    speed = Clamp(speed, 0, 75.0f);
    p->pos = Vector2MoveTowards(p->pos, mouseWorld, speed * GetFrameTime());
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

void SpawnRandomFood(void *arg, CallaterRef invokeRef)
{
    FoodData *foodData = arg;
    if(foodData->nbFoods >= (sizeof(foodData->foods) / sizeof(foodData->foods[0])))
    {
        return;
    }
    
    CallaterSetRepeatRate(invokeRef, 5 * (foodData->nbFoods / 1000.0f));
    
    float xPercent = 1.0f * rand() / RAND_MAX;
    float yPercent = 1.0f * rand() / RAND_MAX;
    Vector2 newFoodPos = {
        .x = xPercent * windowWidth,
        .y = yPercent * windowHeight
    };
    foodData->foods[foodData->nbFoods++] = newFoodPos;
    
    PlaySound(pop);
}
