#ifndef ENEMY_H
#define ENEMY_H

#include "game.h"
#include "gameobjects/bullet.h"

DECL_GAMEOBJECT(
    Enemy,
    struct EnemyData
    {
        Vector2 spawnPos;
        float fireRate;
        float burstDelay;
        float radius;
        Color color;
        struct BulletData bulletData;
    }
);

#endif