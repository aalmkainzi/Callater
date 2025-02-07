#!/bin/bash


clang -mavx src/inspector.c src/main.c src/gameobjects/enemy.c src/gameobjects/enemy_spawner.c src/gameobjects/bullet.c src/gameobjects/player.c external/raylib-5.5_linux_amd64/lib/libraylib.a -I"external" -I "./external/raylib-5.5_linux_amd64/include" -I "./include" -I "../.." -fsanitize=address,undefined -lm -ldl -lrt -lX11 -o sample2 -ggdb -std=gnu11 -Wall -Wextra

exit $?