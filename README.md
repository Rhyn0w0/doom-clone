# doom-clone — tiny Doom clone in C (Raylib)

Raycaster shooter: textured walls, billboard sprites, 2 enemy types with
simple AI, 2 weapons, 2 levels, doors, exit, HUD, health/ammo pickups.
Renders into a software framebuffer, presented in a Raylib window.

## Prerequisites

- A C11 compiler (`cc`, `gcc`, or `clang`), `make`, `pkg-config`
- Raylib 5.x

## Install Raylib

### macOS (Homebrew)

```sh
brew install raylib
```

### Linux (Debian / Ubuntu) — build from source

```sh
sudo apt install build-essential git cmake pkg-config \
    libasound2-dev libx11-dev libxrandr-dev libxi-dev \
    libgl1-mesa-dev libxcursor-dev libxinerama-dev
git clone --depth 1 --branch 5.5 https://github.com/raysan5/raylib.git
cmake -B raylib/build -S raylib -DCMAKE_BUILD_TYPE=Release
cmake --build raylib/build
sudo cmake --install raylib/build
```

### Linux (Arch)

```sh
sudo pacman -S raylib base-devel cmake pkg-config
```

### Windows (MSYS2)

Install the `mingw-w64-x86_64-raylib` package, then build from an
MSYS2 MinGW shell.

## Build and run

```sh
git clone https://github.com/Rhyn0w0/doom-clone.git
cd doom-clone
make
./doomc [--level 1|2]
```

## Controls

`W`/`S` (or Up/Down) move, `A`/`D` strafe, Left/Right turn,
`SPACE`/`F` fire, `E` open door, `1`/`2` select pistol/shotgun,
`P` pause, `R` restart (when dead or after winning), `Q` or `ESC` quit.

## Game

- Enemies: grunts (red, chase + melee) and spitters (purple, keep range
  + ranged bolts). Gunfire alerts nearby enemies.
- Weapons: pistol (1 ammo, fast) and shotgun (3 ammo, 6-pellet spread).
  Kills drop +2 ammo; bronze boxes give +12, white medkits +25 HP.
- Orange doors open with `E` nearby; walk onto the green EXIT cell to
  take the next level. Finish level 2 to win.

## Files

- `src/map.h/.c` — static level grids (2 levels)
- `src/tex.h/.c` — procedural wall/sprite textures (no assets)
- `src/render.h/.c` — software raycaster (DDA walls, sprites, gun)
- `src/game.h/.c` — player, enemy AI, weapons, doors, pickups, levels
- `src/main.c` — entry point, Raylib window/input/HUD loop
- `Makefile`, `README.md`
