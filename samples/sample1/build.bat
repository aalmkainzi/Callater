@echo off
setlocal

gcc -mavx ..\..\callater.c main.c .\raylib-5.5_win64_mingw-w64\lib\libraylib.a -I "./raylib-5.5_linux_amd64/include" -I ".\include" -I "../.." -lgdi32 -lwinmm -o sample1.exe -ggdb -std=gnu11 -Wall -Wextra

exit /b %errorlevel%
