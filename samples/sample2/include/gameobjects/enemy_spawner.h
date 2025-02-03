#ifndef ENEMY_SPAWNER_H
#define ENEMY_SPAWNER_H

#include "game.h"
#include "gameobjects/enemy.h"

DECL_GAMEOBJECT(
    EnemySpawner,
    struct EnemySpawnerData
    {
        float spawnSpeed;
        struct EnemyData nextEnemy;
    }
);

#endif