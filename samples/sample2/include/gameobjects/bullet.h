#ifndef BULLET_H
#define BULLET_H

#include "game.h"

DECL_GAMEOBJECT(
    Bullet,
    struct BulletData
    {
        Vector2 spawnPos;
        Vector2 target;
        Vector2 dir;
        float radius;
        float speed;
        Color color;
    }
);

#endif