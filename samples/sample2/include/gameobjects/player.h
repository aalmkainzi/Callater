#ifndef PLAYER_H
#define PLAYER_H

#include "raylib.h"
#include "callater.h"
#include "game.h"

DECL_GAMEOBJECT(
    Player,
    struct PlayerData
    {
        Vector2 velocity;
        float radius;
        float speed;
        int8_t health;
        Color textColor;
        CallaterRef colorChangeInvokeRef;
    }
);

extern Player *playerInstance;

void PlayerTakeDamage(uint8_t dmg);
void PlayerHeal(uint8_t healBy);

#endif