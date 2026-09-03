#ifndef DOOMC_GAME_H
#define DOOMC_GAME_H

#include "config.h"
#include "render.h"

/* Enemy types. */
#define E_GRUNT 0 /* chaser, melee */
#define E_SPIT 1 /* keeps range, ranged bolt */

typedef enum {
    ST_TITLE = 0,
    ST_PLAYING,
    ST_PAUSED,
    ST_DEAD,
    ST_WIN
} GamePhase;

typedef struct {
    float fwd;    /* -1..1 forward */
    float strafe; /* -1..1 right */
    float turn;   /* -1..1 turning right */
    int fire;
    int use;
    int w1, w2;   /* weapon select */
    int pause;
    int quit;
    int anykey;   /* any key pressed (leave title/dead screens) */
} Input;

typedef struct {
    float x, y;
    int type;
    int hp, maxhp;
    int alive;
    float atk_cd;
    int flash;
    int alert;
} Enemy;

typedef struct {
    float x, y;
    int kind; /* 0 ammo, 1 medkit */
    int taken;
} Item;

typedef struct {
    int cx, cy;
    int open;
} Door;

typedef struct {
    float px, py, angle;
    int hp, maxhp;
    int ammo;
    int weapon; /* 0 pistol, 1 shotgun */
    float fire_cd;
    int muzzle;
    float hurt_fx;

    int level;
    GamePhase phase;
    int kills;

    Enemy enemies[MAX_ENEMIES];
    int nenemies;
    Item items[MAX_ITEMS];
    int nitems;
    Door doors[MAX_DOORS];
    int ndoors;

    char msg[128];
    float msg_t;
} Game;

void game_init(Game *g, int level);
void game_update(Game *g, const Input *in, float dt);
/* Build sprite list for renderer. Returns count in out (cap MAX_SPRITES). */
int game_sprites(const Game *g, RSprite *out);
/* Door query for the renderer. */
int game_door_open(int cx, int cy, void *ud);
void game_msg(Game *g, const char *m);

#endif
