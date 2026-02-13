@echo off
if exist "%~dp0timer.exe" del "%~dp0timer.exe"
clang  main.c -O2 -o "%~dp0timer.exe" -std=c11 -Wall -mwindows -Wno-missing-braces -L  ./lib/  -static  -lcore -lraylib -lopengl32 -lgdi32 -pthread -lwinmm
cmd /c if exist "%~dp0timer.exe" "%~dp0timer.exe"