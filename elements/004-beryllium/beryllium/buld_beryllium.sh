#! /bin/bash
gcc -std=c17 -c beryllium.c -lm -o beryllium.o
gcc -std=c17 -c beryllium_analyze.c -lm -o beryllium_analyze.o
gcc -std=c17 beryllium.o beryllium_analyze.o -lm -o beryllium_analyze
