#include "tex.h"

uint32_t g_wall[NTEX][TEX_SIZE * TEX_SIZE];
uint32_t g_spr[NSPR_KINDS][SPR_SIZE * SPR_SIZE];

static uint32_t shade(uint32_t c, float f)
{
    uint32_t r = (c >> 16) & 255u, g = (c >> 8) & 255u, b = c & 255u;
    r = (uint32_t)(r * f);
    g = (uint32_t)(g * f);
    b = (uint32_t)(b * f);
    return pack_argb(r, g, b);
}

static void brick(uint32_t *t, uint32_t base, uint32_t mortar)
{
    for (int y = 0; y < TEX_SIZE; y++) {
        for (int x = 0; x < TEX_SIZE; x++) {
            int row = y / 8, off = (row & 1) ? 8 : 0;
            int bx = (x + off) % 16;
            uint32_t c = base;
            if (y % 8 == 0 || bx == 0)
                c = mortar;
            /* per-brick tint + noise */
            uint32_t n = (uint32_t)(((x * 73 + y * 149 + row * 31) % 24));
            c = shade(c, 0.92f + n / 200.0f);
            t[y * TEX_SIZE + x] = c;
        }
    }
}

static void metal(uint32_t *t, uint32_t base)
{
    for (int y = 0; y < TEX_SIZE; y++) {
        for (int x = 0; x < TEX_SIZE; x++) {
            uint32_t c = base;
            if (x % 16 == 0 || y % 16 == 0)
                c = shade(base, 0.6f);
            if (x % 16 == 1 || y % 16 == 1)
                c = shade(base, 1.15f > 1.0f ? base : base); /* keep, highlight below */
            if (x % 16 == 1 || y % 16 == 1)
                c = pack_argb(200, 210, 225);
            uint32_t n = (uint32_t)((x * 37 + y * 91) % 12);
            c = shade(c, 0.95f + n / 240.0f);
            if (((x / 8) + (y / 8)) % 2 == 0)
                c = shade(c, 0.94f);
            t[y * TEX_SIZE + x] = c;
        }
    }
}

static void tech(uint32_t *t)
{
    for (int y = 0; y < TEX_SIZE; y++) {
        for (int x = 0; x < TEX_SIZE; x++) {
            uint32_t c = pack_argb(28, 52, 40);
            if (y < 6 || y >= TEX_SIZE - 6)
                c = pack_argb(20, 90, 70);
            if ((x % 32) < 3)
                c = pack_argb(60, 140, 110);
            if (x % 16 == 8 && y % 16 == 8)
                c = pack_argb(120, 255, 180); /* lamp */
            if ((x + y) % 7 == 0)
                c = shade(c, 0.9f);
            t[y * TEX_SIZE + x] = c;
        }
    }
}

static void door(uint32_t *t)
{
    for (int y = 0; y < TEX_SIZE; y++) {
        for (int x = 0; x < TEX_SIZE; x++) {
            uint32_t c = pack_argb(150, 95, 30);
            if (x < 4 || x >= TEX_SIZE - 4)
                c = pack_argb(60, 40, 20);
            if (y % 16 < 3)
                c = pack_argb(210, 160, 60); /* hazard stripes base */
            if (y % 16 < 3 && ((x / 8) % 2 == 0))
                c = pack_argb(40, 30, 20);
            if (x == TEX_SIZE / 2 - 1 || x == TEX_SIZE / 2)
                c = pack_argb(30, 25, 25); /* center seam */
            if (y > 26 && y < 38 && x > 20 && x < 44)
                c = pack_argb(255, 220, 120); /* window */
            t[y * TEX_SIZE + x] = c;
        }
    }
}

static void exittex(uint32_t *t)
{
    for (int y = 0; y < TEX_SIZE; y++) {
        for (int x = 0; x < TEX_SIZE; x++) {
            uint32_t c = pack_argb(30, 120, 60);
            if (x < 4 || x >= TEX_SIZE - 4 || y < 4 || y >= TEX_SIZE - 4)
                c = pack_argb(15, 60, 35);
            if (y > 12 && y < 26) {
                /* blocky "EXIT" band */
                int bar = ((x / 4) % 2 == 0);
                c = bar ? pack_argb(120, 255, 160) : pack_argb(20, 80, 45);
            }
            if (((x * 13 + y * 7) % 11) == 0)
                c = shade(c, 1.1f > 1.0f ? 1.0f : 1.0f);
            t[y * TEX_SIZE + x] = c;
        }
    }
}

#define TRANS 0xFF000000u

static void clear_spr(int k)
{
    for (int i = 0; i < SPR_SIZE * SPR_SIZE; i++)
        g_spr[k][i] = TRANS;
}

static void draw_rect(int k, int x0, int y0, int x1, int y1, uint32_t c)
{
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            if (x < 0 || x >= SPR_SIZE || y < 0 || y >= SPR_SIZE)
                continue;
            g_spr[k][y * SPR_SIZE + x] = c;
        }
    }
}

/* Grunt: bulky red/brown humanoid. Spitter: lean purple fiend. */
static void humanoid(int k, uint32_t skin, uint32_t shirt, uint32_t pants)
{
    clear_spr(k);
    /* legs */
    draw_rect(k, 11, 22, 14, 30, pants);
    draw_rect(k, 17, 22, 20, 30, pants);
    /* torso */
    draw_rect(k, 10, 13, 21, 22, shirt);
    /* arms */
    draw_rect(k, 7, 14, 9, 22, skin);
    draw_rect(k, 22, 14, 24, 22, skin);
    /* head */
    draw_rect(k, 12, 5, 19, 12, skin);
    /* eyes */
    draw_rect(k, 13, 8, 14, 9, pack_argb(255, 40, 40));
    draw_rect(k, 17, 8, 18, 9, pack_argb(255, 40, 40));
    if (k == SPR_SPIT) {
        /* horns */
        draw_rect(k, 9, 1, 11, 5, pack_argb(220, 200, 160));
        draw_rect(k, 20, 1, 22, 5, pack_argb(220, 200, 160));
        /* mouth glow */
        draw_rect(k, 14, 11, 17, 12, pack_argb(180, 60, 220));
    } else {
        /* helmet brim */
        draw_rect(k, 11, 4, 20, 5, pack_argb(90, 60, 40));
    }
}

static void pickup_spr(int k, uint32_t box, uint32_t glyph)
{
    clear_spr(k);
    /* shadow ellipse */
    draw_rect(k, 7, 27, 24, 29, pack_argb(20, 20, 20));
    draw_rect(k, 9, 14, 22, 27, box);
    draw_rect(k, 9, 14, 22, 16, pack_argb(255, 255, 255));
    if (k == SPR_AMMO) {
        /* bullets */
        draw_rect(k, 12, 18, 14, 25, glyph);
        draw_rect(k, 16, 18, 18, 25, glyph);
        draw_rect(k, 12, 18, 14, 20, pack_argb(255, 230, 150));
        draw_rect(k, 16, 18, 18, 20, pack_argb(255, 230, 150));
    } else {
        /* cross */
        draw_rect(k, 14, 17, 17, 25, glyph);
        draw_rect(k, 11, 20, 20, 23, glyph);
    }
}

void tex_init(void)
{
    brick(g_wall[0], pack_argb(150, 70, 55), pack_argb(90, 85, 80));
    metal(g_wall[1], pack_argb(110, 125, 145));
    tech(g_wall[2]);
    door(g_wall[3]);
    exittex(g_wall[4]);
    humanoid(SPR_GRUNT, pack_argb(200, 150, 120), pack_argb(140, 40, 35),
             pack_argb(70, 55, 50));
    humanoid(SPR_SPIT, pack_argb(150, 110, 200), pack_argb(90, 40, 130),
             pack_argb(50, 30, 70));
    pickup_spr(SPR_AMMO, pack_argb(60, 70, 60), pack_argb(230, 190, 90));
    pickup_spr(SPR_MEDI, pack_argb(200, 210, 215), pack_argb(220, 50, 50));
}
