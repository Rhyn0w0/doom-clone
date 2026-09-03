#ifndef DOOMC_RENDER_H
#define DOOMC_RENDER_H

#include <stdint.h>
#include "config.h"

/* Billboard to draw (enemies, items). kind = SPR_* from tex.h. */
typedef struct {
    float x, y;
    int kind;
    int flash; /* >0: draw white flash (recently hit) */
} RSprite;

/* Door query: returns 1 if the door cell (cx,cy) is open. */
typedef int (*door_open_fn)(int cx, int cy, void *ud);

/* Render one frame into pixels (FB_W*FB_H ARGB8888).
 * zbuf must hold FB_W floats (per-column wall distance).
 * weapon: 0 pistol, 1 shotgun. muzzle: frames left of muzzle flash. */
void render_frame(uint32_t *pixels, float *zbuf,
                  float px, float py, float angle, int level,
                  door_open_fn doors, void *door_ud,
                  const RSprite *spr, int nspr,
                  int weapon, int muzzle);

#endif
