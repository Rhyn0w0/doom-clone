#ifndef DOOMC_CONFIG_H
#define DOOMC_CONFIG_H

/* Framebuffer resolution (software renderer, ARGB8888). */
/* The Raylib window shows it scaled up (see WIN_SCALE in main.c). */
#define FB_W 240
#define FB_H 120

#define TEX_SIZE 64
#define SPR_SIZE 32

/* Camera plane half-width: tan(fov/2); 0.66 ~= 66 deg FOV. */
#define CAM_PLANE 0.66f

#define MAX_ENEMIES 32
#define MAX_ITEMS 32
#define MAX_DOORS 32
#define MAX_SPRITES (MAX_ENEMIES + MAX_ITEMS)

#define PLAYER_RADIUS 0.22f
#define PLAYER_HP 100
#define START_AMMO 30

#endif
