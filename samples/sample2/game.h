#include <stdint.h>
#include "raylib.h"

extern float playerRadius;

typedef struct Player
{
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
    
    Vector2 bounds;
} GameState;

extern GameState gameState;
