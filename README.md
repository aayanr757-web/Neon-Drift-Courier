# Neon Drift Courier (Elite)

A fast neon arcade survival game written in C using raylib.
Collect score orbs, avoid enemies, and use shield power-ups to survive longer.

## How to Play
- Move: WASD or Arrow keys
- Pause/Resume: P
- Menu (from pause/game over): M
- Restart (game over): ENTER or SPACE
- Quit: ESC

## Rules
- Collect the pink ORB to increase your score.
- Enemies get harder as your score increases.
- Collect the cyan ORB to activate a temporary shield.
- If you collide with an enemy without shield → Game Over.

## Build (Windows - MSYS2 UCRT64)

cd /c/Users/aayan/OneDrive/Desktop/Game-Project
gcc src/main.c src/game.c -o game.exe -lraylib -lopengl32 -lgdi32 -lwinmm

## Run

./game.exe

## Features

- Neon-style animated graphics with glow effects
- Particle system for visual feedback
- Dynamic enemy spawning (difficulty increases over time)
- Shield power-up mechanic (cyan ORB)
- Screen shake and slow-motion effects on collisions
- Smooth player movement using delta time

## Author

Aayan Raza  
Neon Drift Courier (Elite)
