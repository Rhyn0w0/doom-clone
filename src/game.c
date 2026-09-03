#include "game.h"
#include "map.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define GRUNT_HP 40
#define SPIT_HP 25
#define GRUNT_DMG 9
#define SPIT_DMG 7
#define PISTOL_DMG 14
#define SHOT_DMG 9
#define SHOT_PELLETS 6

void game_msg(Game *g, const char *m)
{
    strncpy(g->msg, m, sizeof(g->msg) - 1);
    g->msg[sizeof(g->msg) - 1] = '\0';
    g->msg_t = 3.0f;
}

static int door_idx(const Game *g, int cx, int cy)
{
    for (int i = 0; i < g->ndoors; i++) {
        if (g->doors[i].cx == cx && g->doors[i].cy == cy)
            return i;
    }
    return -1;
}

int game_door_open(int cx, int cy, void *ud)
{
    const Game *g = (const Game *)ud;
    int i = door_idx(g, cx, cy);
    return (i >= 0 && g->doors[i].open) ? 1 : 0;
}

static int solid_for_move(const Game *g, int cx, int cy)
{
    char c = map_cell(g->level, cx, cy);
    if (c == 'D') {
        int i = door_idx(g, cx, cy);
        if (i >= 0 && g->doors[i].open)
            return 0;
        return 1;
    }
    /* walls ('X' exit is walkable) */
    if (c == '#' || c == 'B' || c == 'C')
        return 1;
    if (c == ' ')
        return 1;
    return 0;
}

static int collide(const Game *g, float x, float y, float r)
{
    int x0 = (int)floorf(x - r), x1 = (int)floorf(x + r);
    int y0 = (int)floorf(y - r), y1 = (int)floorf(y + r);
    for (int cy = y0; cy <= y1; cy++) {
        for (int cx = x0; cx <= x1; cx++) {
            if (!solid_for_move(g, cx, cy))
                continue;
            float nx = x < cx ? (float)cx : (x > cx + 1 ? (float)(cx + 1) : x);
            float ny = y < cy ? (float)cy : (y > cy + 1 ? (float)(cy + 1) : y);
            float dx = x - nx, dy = y - ny;
            if (dx * dx + dy * dy < r * r)
                return 1;
        }
    }
    return 0;
}

/* LOS via DDA on movement solidity. */
static int los(const Game *g, float x0, float y0, float x1, float y1)
{
    float dx = x1 - x0, dy = y1 - y0;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist < 1e-6f)
        return 1;
    int steps = (int)(dist * 8.0f) + 1;
    for (int i = 1; i < steps; i++) {
        float t = (float)i / (float)steps;
        int cx = (int)(x0 + dx * t), cy = (int)(y0 + dy * t);
        if (solid_for_move(g, cx, cy))
            return 0;
    }
    return 1;
}

static float frand(void)
{
    return (float)rand() / (float)RAND_MAX;
}

void game_init(Game *g, int level)
{
    memset(g, 0, sizeof(*g));
    g->level = level;
    g->phase = ST_TITLE;
    g->hp = g->maxhp = PLAYER_HP;
    g->ammo = START_AMMO;
    g->weapon = 0;
    g->px = 1.5f;
    g->py = 1.5f;
    g->angle = 0.0f;
    snprintf(g->msg, sizeof(g->msg),
             level == 0 ? "Level 1: find the EXIT (green). E opens doors."
                        : "Level 2: they know you're here.");

    int W = map_width(level), H = map_height(level);
    int ang_set = 0;
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            char c = map_cell(level, x, y);
            float cx = x + 0.5f, cy = y + 0.5f;
            if (c == 'P' && !ang_set) {
                g->px = cx;
                g->py = cy;
                g->angle = 0.0f;
                ang_set = 1;
            } else if ((c == 'G' || c == 'S') && g->nenemies < MAX_ENEMIES) {
                Enemy *e = &g->enemies[g->nenemies++];
                e->x = cx;
                e->y = cy;
                e->type = (c == 'G') ? E_GRUNT : E_SPIT;
                e->maxhp = e->hp = (c == 'G') ? GRUNT_HP : SPIT_HP;
                e->alive = 1;
            } else if ((c == 'a' || c == 'h') && g->nitems < MAX_ITEMS) {
                Item *it = &g->items[g->nitems++];
                it->x = cx;
                it->y = cy;
                it->kind = (c == 'a') ? 0 : 1;
            } else if (c == 'D' && g->ndoors < MAX_DOORS) {
                if (door_idx(g, x, y) < 0) {
                    g->doors[g->ndoors].cx = x;
                    g->doors[g->ndoors].cy = y;
                    g->doors[g->ndoors].open = 0;
                    g->ndoors++;
                }
            }
        }
    }
}

static void hurt_player(Game *g, int dmg, const char *src)
{
    if (g->phase != ST_PLAYING)
        return;
    g->hp -= dmg;
    g->hurt_fx = 1.0f;
    if (g->hp <= 0) {
        g->hp = 0;
        g->phase = ST_DEAD;
        char b[128];
        snprintf(b, sizeof(b), "Killed by %s. Press any key.", src);
        game_msg(g, b);
    }
}

static void fire_weapon(Game *g)
{
    if (g->fire_cd > 0.0f || g->phase != ST_PLAYING)
        return;
    int pellets = 1, dmg = PISTOL_DMG, cost = 1;
    float spread = 0.03f, cd = 0.28f;
    if (g->weapon == 1) {
        pellets = SHOT_PELLETS;
        dmg = SHOT_DMG;
        cost = 3;
        spread = 0.09f;
        cd = 0.85f;
    }
    if (g->ammo < cost) {
        game_msg(g, "NO AMMO! Grab bronze boxes. (1: pistol)");
        g->fire_cd = 0.25f;
        return;
    }
    g->ammo -= cost;
    g->fire_cd = cd;
    g->muzzle = 4;

    for (int p = 0; p < pellets; p++) {
        float a = g->angle + (frand() * 2.0f - 1.0f) * spread;
        float dx = cosf(a), dy = sinf(a);
        /* nearest enemy roughly along this ray */
        Enemy *best = NULL;
        float bestd = 9.0f;
        for (int i = 0; i < g->nenemies; i++) {
            Enemy *e = &g->enemies[i];
            if (!e->alive)
                continue;
            float ex = e->x - g->px, ey = e->y - g->py;
            float along = ex * dx + ey * dy;
            if (along < 0.2f || along > bestd)
                continue;
            float perp = fabsf(ex * dy - ey * dx);
            if (perp > 0.28f + along * 0.03f)
                continue;
            if (!los(g, g->px, g->py, e->x, e->y))
                continue;
            best = e;
            bestd = along;
        }
        if (best) {
            int fall = bestd > 5.0f ? dmg / 2 : dmg;
            if (fall < 1)
                fall = 1;
            best->hp -= fall;
            best->flash = 3;
            best->alert = 1;
            if (best->hp <= 0) {
                best->alive = 0;
                g->kills++;
                g->ammo += 2; /* scavenge */
                game_msg(g, "Enemy down! +2 ammo.");
            }
        }
    }
    /* gunshot alerts nearby enemies */
    for (int i = 0; i < g->nenemies; i++) {
        Enemy *e = &g->enemies[i];
        if (!e->alive)
            continue;
        float dx = e->x - g->px, dy = e->y - g->py;
        if (dx * dx + dy * dy < 64.0f)
            e->alert = 1;
    }
}

static void use_door(Game *g)
{
    float bestd = 1.9f;
    int best = -1;
    for (int i = 0; i < g->ndoors; i++) {
        if (g->doors[i].open)
            continue;
        float dx = (g->doors[i].cx + 0.5f) - g->px;
        float dy = (g->doors[i].cy + 0.5f) - g->py;
        float d = sqrtf(dx * dx + dy * dy);
        if (d < bestd) {
            bestd = d;
            best = i;
        }
    }
    if (best >= 0) {
        g->doors[best].open = 1;
        game_msg(g, "Door opened.");
    } else {
        game_msg(g, "No door in reach. (E near orange doors)");
    }
}

static void update_enemy(Game *g, Enemy *e, float dt)
{
    if (!e->alive)
        return;
    if (e->flash > 0)
        e->flash--;
    if (e->atk_cd > 0.0f)
        e->atk_cd -= dt;

    float dx = g->px - e->x, dy = g->py - e->y;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist < 1e-5f)
        return;
    int seen = los(g, e->x, e->y, g->px, g->py);
    if (seen && dist < 10.0f)
        e->alert = 1;
    if (!e->alert)
        return;

    float speed = (e->type == E_GRUNT) ? 2.1f : 1.5f;
    float want = dist;
    if (e->type == E_GRUNT) {
        if (dist < 0.75f) {
            /* melee attack */
            if (e->atk_cd <= 0.0f && seen) {
                e->atk_cd = 1.0f;
                hurt_player(g, GRUNT_DMG, "a grunt");
            }
            return;
        }
    } else {
        /* spitter: hold ~4 units, fire bolts */
        if (dist < 6.5f && seen && e->atk_cd <= 0.0f) {
            e->atk_cd = 1.6f;
            /* dodge chance grows with distance */
            if (frand() > dist * 0.06f)
                hurt_player(g, SPIT_DMG, "a spitter bolt");
            else
                game_msg(g, "Bolt missed!");
            return;
        }
        if (dist < 2.5f) {
            want = -1.0f; /* back off: flip direction */
            dx = -dx;
            dy = -dy;
            dist = sqrtf(dx * dx + dy * dy);
        } else if (dist < 4.5f && seen) {
            return; /* in the pocket: stand and shoot */
        }
        (void)want;
    }

    /* chase (or retreat) step with slide */
    float nx = e->x + (dx / dist) * speed * dt;
    float ny = e->y + (dy / dist) * speed * dt;
    /* separation from other enemies */
    for (int i = 0; i < g->nenemies; i++) {
        Enemy *o = &g->enemies[i];
        if (o == e || !o->alive)
            continue;
        float ox = nx - o->x, oy = ny - o->y;
        float od = sqrtf(ox * ox + oy * oy);
        if (od < 0.45f && od > 1e-4f) {
            nx += (ox / od) * (0.45f - od) * 0.5f;
            ny += (oy / od) * (0.45f - od) * 0.5f;
        }
    }
    if (!collide(g, nx, e->y, 0.22f))
        e->x = nx;
    if (!collide(g, e->x, ny, 0.22f))
        e->y = ny;
    /* touch damage from spitters too */
    if (e->type == E_SPIT && dist < 0.7f && e->atk_cd <= 0.0f && seen) {
        e->atk_cd = 1.2f;
        hurt_player(g, 5, "a spitter's claws");
    }
}

void game_update(Game *g, const Input *in, float dt)
{
    if (g->msg_t > 0.0f)
        g->msg_t -= dt;
    if (g->fire_cd > 0.0f)
        g->fire_cd -= dt;
    if (g->muzzle > 0)
        g->muzzle--;
    if (g->hurt_fx > 0.0f)
        g->hurt_fx -= dt * 2.0f;

    switch (g->phase) {
    case ST_TITLE:
        if (in->anykey || in->fire || in->use) {
            g->phase = ST_PLAYING;
            game_msg(g, "WASD move, arrows turn, SPACE fire, E door, 1/2 guns.");
        }
        return;
    case ST_PAUSED:
        if (in->pause && g->msg_t <= 0.0f) {
            /* debounce handled by caller edge; here just resume */
        }
        return;
    case ST_DEAD:
    case ST_WIN:
        return;
    case ST_PLAYING:
        break;
    }

    if (in->quit) {
        g->phase = ST_WIN; /* caller interprets quit separately */
        return;
    }

    /* weapon select */
    if (in->w1) {
        g->weapon = 0;
        game_msg(g, "Pistol.");
    }
    if (in->w2) {
        g->weapon = 1;
        game_msg(g, "Shotgun (3 shells).");
    }

    /* turning + movement */
    g->angle += in->turn * 2.6f * dt;
    while (g->angle > (float)M_PI)
        g->angle -= 2.0f * (float)M_PI;
    while (g->angle <= -(float)M_PI)
        g->angle += 2.0f * (float)M_PI;

    float fx = cosf(g->angle), fy = sinf(g->angle);
    float rx = -fy, ry = fx;
    float speed = 3.4f;
    float mx = (fx * in->fwd + rx * in->strafe) * speed * dt;
    float my = (fy * in->fwd + ry * in->strafe) * speed * dt;
    float nx = g->px + mx;
    if (!collide(g, nx, g->py, PLAYER_RADIUS))
        g->px = nx;
    float ny = g->py + my;
    if (!collide(g, g->px, ny, PLAYER_RADIUS))
        g->py = ny;

    if (in->fire)
        fire_weapon(g);
    if (in->use)
        use_door(g);

    /* enemies */
    for (int i = 0; i < g->nenemies; i++)
        update_enemy(g, &g->enemies[i], dt);

    /* pickups */
    for (int i = 0; i < g->nitems; i++) {
        Item *it = &g->items[i];
        if (it->taken)
            continue;
        float dx = it->x - g->px, dy = it->y - g->py;
        if (dx * dx + dy * dy < 0.25f) {
            it->taken = 1;
            if (it->kind == 0) {
                g->ammo += 12;
                game_msg(g, "+12 ammo.");
            } else {
                g->hp += 25;
                if (g->hp > g->maxhp)
                    g->hp = g->maxhp;
                game_msg(g, "+25 health.");
            }
        }
    }

    /* exit check: standing on X cell */
    {
        int cx = (int)g->px, cy = (int)g->py;
        if (map_cell(g->level, cx, cy) == 'X') {
            if (g->level + 1 < MAP_COUNT) {
                int keep_hp = g->hp, keep_ammo = g->ammo + 10, keep_w = g->weapon;
                int keep_k = g->kills;
                game_init(g, g->level + 1);
                g->phase = ST_PLAYING;
                g->hp = keep_hp + 10 > g->maxhp ? g->maxhp : keep_hp + 10;
                g->ammo = keep_ammo;
                g->weapon = keep_w;
                g->kills = keep_k;
                game_msg(g, "Next level! +10 ammo, +10 HP.");
            } else {
                g->phase = ST_WIN;
                game_msg(g, "You escaped! You win.");
            }
        }
    }
}

int game_sprites(const Game *g, RSprite *out)
{
    int n = 0;
    for (int i = 0; i < g->nenemies && n < MAX_SPRITES; i++) {
        const Enemy *e = &g->enemies[i];
        if (!e->alive)
            continue;
        out[n].x = e->x;
        out[n].y = e->y;
        out[n].kind = (e->type == E_GRUNT) ? 0 : 1;
        out[n].flash = e->flash > 0;
        n++;
    }
    for (int i = 0; i < g->nitems && n < MAX_SPRITES; i++) {
        const Item *it = &g->items[i];
        if (it->taken)
            continue;
        out[n].x = it->x;
        out[n].y = it->y;
        out[n].kind = it->kind == 0 ? 2 : 3;
        out[n].flash = 0;
        n++;
    }
    return n;
}
