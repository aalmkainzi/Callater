#!/bin/bash

gcc -mavx ../../callater.c main.c ./raylib-5.5_linux64/lib/libraylib.a -I "./raylib-5.5_linux_amd64/include" -I "./include" -I "../.." -ldl -lrt -lX11 -o sample1 -ggdb -std=gnu11 -Wall -Wextra

exit $?
