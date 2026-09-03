#include "render.h"
#include "map.h"
#include "tex.h"
#include <math.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Map a wall cell char to texture index 0..NTEX-1, or -1 if not a wall. */
static int wall_tex(char c, int is_open_door)
{
    switch (c) {
    case '#': return 0;
    case 'B': return 1;
    case 'C': return 2;
    case 'D': return is_open_door ? -1 : 3;
    case 'X': return 4;
    default: return -1;
    }
}

/* Is cell (cx,cy) solid for rays? Open doors are not walls. */
static int solid_at(int level, int cx, int cy, door_open_fn doors, void *ud)
{
    char c = map_cell(level, cx, cy);
    if (c == 'D' && doors && doors(cx, cy, ud))
        return 0;
    return wall_tex(c, 0) >= 0;
}

static uint32_t shade_px(uint32_t c, float f)
{
    if (f < 0.0f)
        f = 0.0f;
    if (f > 1.0f)
        f = 1.0f;
    uint32_t r = (uint32_t)(((c >> 16) & 255u) * f);
    uint32_t g = (uint32_t)(((c >> 8) & 255u) * f);
    uint32_t b = (uint32_t)((c & 255u) * f);
    return 0xFF000000u | (r << 16) | (g << 8) | b;
}

typedef struct {
    float dist;
    int idx;
} SortItem;

static int cmp_sort(const void *a, const void *b)
{
    float da = ((const SortItem *)a)->dist;
    float db = ((const SortItem *)b)->dist;
    if (da < db)
        return 1;
    if (da > db)
        return -1;
    return 0;
}

void render_frame(uint32_t *pixels, float *zbuf,
                  float px, float py, float angle, int level,
                  door_open_fn doors, void *door_ud,
                  const RSprite *spr, int nspr,
                  int weapon, int muzzle)
{
    float dirx = cosf(angle), diry = sinf(angle);
    float planex = -diry * CAM_PLANE, planey = dirx * CAM_PLANE;

    uint32_t ceilu = pack_argb(12, 12, 22);
    uint32_t ceill = pack_argb(20, 20, 34);
    uint32_t flooru = pack_argb(42, 36, 30);
    uint32_t floorl = pack_argb(22, 19, 16);

    for (int x = 0; x < FB_W; x++) {
        float cam = 2.0f * x / (float)FB_W - 1.0f;
        float rdx = dirx + planex * cam;
        float rdy = diry + planey * cam;

        int mx = (int)px, my = (int)py;
        float ddx = rdx == 0 ? 1e30f : fabsf(1.0f / rdx);
        float ddy = rdy == 0 ? 1e30f : fabsf(1.0f / rdy);
        int sx, sy, stepx_signed, stepy_signed;
        float sdx, sdy;
        if (rdx < 0) {
            stepx_signed = -1;
            sdx = (px - mx) * ddx;
        } else {
            stepx_signed = 1;
            sdx = (mx + 1.0f - px) * ddx;
        }
        if (rdy < 0) {
            stepy_signed = -1;
            sdy = (py - my) * ddy;
        } else {
            stepy_signed = 1;
            sdy = (my + 1.0f - py) * ddy;
        }
        sx = stepx_signed;
        sy = stepy_signed;
        int side = 0, hit = 0, guard = 0;
        while (!hit && guard++ < 128) {
            if (sdx < sdy) {
                sdx += ddx;
                mx += sx;
                side = 0;
            } else {
                sdy += ddy;
                my += sy;
                side = 1;
            }
            if (solid_at(level, mx, my, doors, door_ud))
                hit = 1;
        }
        float dist = side == 0 ? (sdx - ddx) : (sdy - ddy);
        if (dist < 0.02f)
            dist = 0.02f;
        zbuf[x] = dist;

        /* wall texture coordinate */
        float wallx = side == 0 ? (py + dist * rdy) : (px + dist * rdx);
        wallx -= floorf(wallx);
        int texx = (int)(wallx * TEX_SIZE);
        if ((side == 0 && rdx > 0) || (side == 1 && rdy < 0))
            texx = TEX_SIZE - 1 - texx;
        if (texx < 0)
            texx = 0;
        if (texx >= TEX_SIZE)
            texx = TEX_SIZE - 1;

        char c = map_cell(level, mx, my);
        int is_open = (c == 'D' && doors && doors(mx, my, door_ud));
        int ti = wall_tex(c, is_open);
        if (ti < 0)
            ti = 0;

        int lineh = (int)(FB_H / dist);
        int y0 = FB_H / 2 - lineh / 2;
        int y1 = FB_H / 2 + lineh / 2;
        if (y0 < 0)
            y0 = 0;
        if (y1 >= FB_H)
            y1 = FB_H - 1;

        float bright = 1.0f / (1.0f + dist * dist * 0.12f);
        if (side == 1)
            bright *= 0.72f;

        for (int y = 0; y < FB_H; y++) {
            uint32_t col;
            if (y < y0) {
                float t = (float)y / (float)(FB_H / 2);
                col = t < 0.5f ? ceilu : ceill;
                col = shade_px(col, 0.7f + 0.3f * t);
            } else if (y > y1) {
                float t = (float)(y - FB_H / 2) / (float)(FB_H / 2);
                col = t < 0.5f ? flooru : floorl;
                col = shade_px(col, (1.1f - 0.6f * t) * bright + 0.15f);
            } else {
                int texy = ((y - (FB_H / 2 - lineh / 2)) * TEX_SIZE) / lineh;
                if (texy < 0)
                    texy = 0;
                if (texy >= TEX_SIZE)
                    texy = TEX_SIZE - 1;
                col = shade_px(g_wall[ti][texy * TEX_SIZE + texx], bright);
            }
            pixels[y * FB_W + x] = col;
        }
    }

    /* Sprites, far to near. */
    static SortItem order[MAX_SPRITES];
    if (nspr > MAX_SPRITES)
        nspr = MAX_SPRITES;
    for (int i = 0; i < nspr; i++) {
        float dx = spr[i].x - px, dy = spr[i].y - py;
        order[i].dist = dx * dx + dy * dy;
        order[i].idx = i;
    }
    qsort(order, (size_t)nspr, sizeof(order[0]), cmp_sort);

    /* Camera transform: rel = depth*dir + lat*plane. The solution's
     * first component is depth (along dir), the second is lateral
     * (along plane) — do not swap them. */
    float m00 = dirx, m01 = planex, m10 = diry, m11 = planey;
    float det = m00 * m11 - m01 * m10;

    for (int s = 0; s < nspr; s++) {
        const RSprite *sp = &spr[order[s].idx];
        float relx = sp->x - px, rely = sp->y - py;
        if (det == 0.0f)
            continue;
        float depth = (m11 * relx - m01 * rely) / det;
        float lat = (-m10 * relx + m00 * rely) / det;
        if (depth <= 0.12f)
            continue; /* behind camera */
        int scrx = (int)((FB_W / 2) * (1.0f + lat / depth));
        float scale = sp->kind < 2 ? 0.9f : 0.45f;
        int size = (int)(fabsf(FB_H / depth) * scale);
        int w = size, h = size;
        int x0 = scrx - w / 2, x1 = scrx + w / 2;
        int y0 = FB_H / 2 - h / 2 + (int)(FB_H / (depth * 8.0f));
        int y1 = y0 + h;
        float bright = 1.0f / (1.0f + depth * depth * 0.12f);
        for (int x = x0; x <= x1; x++) {
            if (x < 0 || x >= FB_W)
                continue;
            if (depth >= zbuf[x])
                continue;
            int u = (x - x0) * SPR_SIZE / (w + 1);
            if (u < 0)
                u = 0;
            if (u >= SPR_SIZE)
                u = SPR_SIZE - 1;
            for (int y = y0; y <= y1; y++) {
                if (y < 0 || y >= FB_H)
                    continue;
                int v = (y - y0) * SPR_SIZE / (h + 1);
                if (v < 0)
                    v = 0;
                if (v >= SPR_SIZE)
                    v = SPR_SIZE - 1;
                uint32_t texel = g_spr[sp->kind][v * SPR_SIZE + u];
                if (texel == 0xFF000000u)
                    continue; /* transparent */
                uint32_t col = shade_px(texel, bright);
                if (sp->flash)
                    col = pack_argb(255, 240, 240);
                pixels[y * FB_W + x] = col;
            }
        }
    }

    /* Viewmodel gun: centered bottom rect, per-weapon shape + flash. */
    {
        int gw = weapon == 1 ? 26 : 14;
        int gh = weapon == 1 ? 34 : 44;
        int gx0 = FB_W / 2 - gw / 2, gx1 = FB_W / 2 + gw / 2;
        int gy1 = FB_H - 1, gy0 = FB_H - gh;
        for (int y = gy0; y <= gy1; y++) {
            if (y < 0 || y >= FB_H)
                continue;
            for (int x = gx0; x <= gx1; x++) {
                if (x < 0 || x >= FB_W)
                    continue;
                int edge = (x == gx0 || x == gx1) ? 1 : 0;
                uint32_t col = edge ? pack_argb(25, 25, 30)
                                    : pack_argb(105, 100, 95);
                if (weapon == 1 && y < gy0 + 10)
                    col = edge ? pack_argb(60, 30, 25)
                               : pack_argb(140, 70, 50); /* double barrel */
                if (muzzle > 0 && y < gy0 + 12 && abs(x - FB_W / 2) < 8)
                    col = pack_argb(255, 220, 120);
                pixels[y * FB_W + x] = col;
            }
        }
        if (muzzle > 0) {
            /* crosshair flash dot */
            for (int dy = -2; dy <= 2; dy++) {
                for (int dx = -2; dx <= 2; dx++) {
                    int x = FB_W / 2 + dx, y = FB_H / 2 + dy;
                    pixels[y * FB_W + x] = pack_argb(255, 255, 200);
                }
            }
        } else {
            pixels[(FB_H / 2) * FB_W + FB_W / 2] = pack_argb(255, 255, 255);
        }
    }
}
