#ifndef DOOMC_TEX_H
#define DOOMC_TEX_H

#include <stdint.h>
#include "config.h"

/* Wall texture ids (1-based on maps; 0 = no wall). */
#define NTEX 5
/* Sprite kinds. */
#define SPR_GRUNT 0
#define SPR_SPIT 1
#define SPR_AMMO 2
#define SPR_MEDI 3
#define NSPR_KINDS 4

extern uint32_t g_wall[NTEX][TEX_SIZE * TEX_SIZE];
/* 0xFF000000 in a texel = transparent. */
extern uint32_t g_spr[NSPR_KINDS][SPR_SIZE * SPR_SIZE];

void tex_init(void);

static inline uint32_t pack_argb(uint32_t r, uint32_t g, uint32_t b)
{
    return 0xFF000000u | ((r & 255u) << 16) | ((g & 255u) << 8) | (b & 255u);
}

#endif
