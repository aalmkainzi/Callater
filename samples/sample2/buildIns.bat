@echo off
setlocal

gcc -mavx src\main.c src\gameobjects\enemy.c src\gameobjects\enemy_spawner.c src\gameobjects\bullet.c src\gameobjects\player.c src\inspector.c .\external\raylib-5.5_win64_mingw-w64\lib\libraylib.a -I "./external" -I "./external/raylib-5.5_win64_mingw-w64/include" -I ".\include" -I "../.." -lgdi32 -lwinmm -o sample2.exe -ggdb -std=gnu11 -Wall -Wextra

exit /b %errorlevel%
