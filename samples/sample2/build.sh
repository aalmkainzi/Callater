#!/bin/bash

gcc -mavx ../../callater.c src/main.c src/gameobjects/enemy.c src/gameobjects/enemySpawner.c src/gameobjects/bullet.c src/gameobjects/player.c src/game.c ./raylib-5.5_linux64/lib/libraylib.a -I "./raylib-5.5_linux_amd64/include" -I "./include" -I "../.." -ldl -lrt -lX11 -o sample2 -ggdb -std=gnu11 -Wall -Wextra

exit $?
