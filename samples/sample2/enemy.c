#include "game.h"
#include "../../callater.h"

static uint64_t nextId = 1;

void ShootAtPlayer(void *arg, CallaterRef invokeRef);

void SpawnEnemy(void *arg, CallaterRef invokeRef)
{
    Enemy *enemy = arg;
    enemy->id = nextId++;
    InvokeRepeatID(ShootAtPlayer, enemy, 1.0f, enemy->fireRate, enemy->id);
}

void ShootAtPlayer(void *arg, CallaterRef invokeRef)
{
    Enemy *enemy = arg;
    
}
