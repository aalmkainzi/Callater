#include <stdint.h>
#include "raylib.h"

extern float playerRadius;

typedef struct Drawable
{
    void *arg;
    void(*draw)(void*arg);
} Drawable;

typedef struct Player
{
    Drawable drawPlayer;
    Vector2 pos;
    Vector2 velocity;
    float speed;
    bool readyToFire;
} Player;

typedef struct Enemy
{
    uint64_t id;
    Vector2 pos;
    float fireRate;
    float burstDelay;
} Enemy;

typedef struct GameState
{
    Player *player;
    
    Enemy *enemies;
    uint64_t nbEnemies;
    uint64_t capEnemies;
    
    Drawable *drawables;
    uint64_t nbDrawables;
    uint64_t capDrawables;
    
    Vector2 bounds;
} GameState;

extern GameState gameState;

uint64_t AddDrawable(Drawable drawable);
