@echo off
if exist "%~dp0timer_debug.exe" del "%~dp0timer_debug.exe"
clang  main.c -O2 -o "%~dp0timer_debug.exe" -std=c11 -Wall -Wno-missing-braces -L  ./lib/  -static  -lcore -lraylib -lopengl32 -lgdi32 -pthread -lwinmm
cmd /c if exist "%~dp0timer_debug.exe" "%~dp0timer_debug.exe"