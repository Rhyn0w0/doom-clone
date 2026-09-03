#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "raylib.h"

#include "game.h"
#include "map.h"
#include "render.h"
#include "tex.h"

#define WIN_SCALE 4
#define HUD_H 76

static void usage(const char *prog)
{
    printf("Usage: %s [options] [level]\n", prog);
    printf("Options:\n");
    printf("  --level N   start at level N (1-%d, default 1)\n", MAP_COUNT);
    printf("  --help      show this help\n");
    printf("Controls: W/S or Up/Down move, A/D strafe, Left/Right turn,\n");
    printf("  SPACE/F fire, E use door, 1/2 weapons, P pause,\n");
    printf("  R restart (when dead), Q or ESC quit.\n");
}

static void poll_input(Input *in, int *pause_pressed, int *quit_pressed)
{
    memset(in, 0, sizeof(*in));
    in->fwd = ((IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) ? 1.0f : 0.0f) -
              ((IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) ? 1.0f : 0.0f);
    in->strafe = (IsKeyDown(KEY_D) ? 1.0f : 0.0f) -
                 (IsKeyDown(KEY_A) ? 1.0f : 0.0f);
    in->turn = (IsKeyDown(KEY_RIGHT) ? 1.0f : 0.0f) -
               (IsKeyDown(KEY_LEFT) ? 1.0f : 0.0f);
    in->fire = IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_F);
    in->use = IsKeyPressed(KEY_E);
    in->w1 = IsKeyPressed(KEY_ONE);
    in->w2 = IsKeyPressed(KEY_TWO);
    *pause_pressed = IsKeyPressed(KEY_P);
    *quit_pressed = IsKeyPressed(KEY_Q);
    if (in->fwd != 0.0f || in->strafe != 0.0f || in->turn != 0.0f ||
        in->fire || in->use || in->w1 || in->w2 ||
        *pause_pressed || *quit_pressed)
        in->anykey = 1;
    else if (GetKeyPressed() != 0)
        in->anykey = 1;
}

static void draw_hud(const Game *g, int top_h)
{
    int alive = 0;
    for (int i = 0; i < g->nenemies; i++)
        if (g->enemies[i].alive)
            alive++;
    char buf[160];
    snprintf(buf, sizeof(buf), "L%d  HP %d/%d  AMMO %d  %s  FOES %d  KILLS %d",
             g->level + 1, g->hp, g->maxhp, g->ammo,
             g->weapon ? "SHOTGUN" : "PISTOL", alive, g->kills);
    DrawText(buf, 12, top_h + 8, 20, RAYWHITE);
    if (g->msg_t > 0.0f)
        DrawText(g->msg, 12, top_h + 34, 18, YELLOW);
    else
        DrawText("WASD move  SPACE fire  E door  1/2 weapon  P pause  Q quit",
                 12, top_h + 34, 18, GRAY);
}

static void draw_center_box(const char *title, const char *sub, int view_h,
                            int view_w)
{
    int w = 460, h = 110;
    int x = (view_w - w) / 2, y = (view_h - h) / 2;
    DrawRectangle(x, y, w, h, Fade(BLACK, 0.8f));
    DrawRectangleLines(x, y, w, h, RED);
    DrawText(title, x + 24, y + 18, 28, RAYWHITE);
    DrawText(sub, x + 24, y + 58, 18, LIGHTGRAY);
}

static int ray_loop(Game *g)
{
    const int view_w = FB_W * WIN_SCALE;
    const int view_h = FB_H * WIN_SCALE;
    InitWindow(view_w, view_h + HUD_H, "doomc");
    SetTargetFPS(60);

    Image img = GenImageColor(FB_W, FB_H, BLANK);
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);

    static uint32_t pixels[FB_W * FB_H];
    static float zbuf[FB_W];
    static RSprite spr[MAX_SPRITES];
    static unsigned char rgba[FB_W * FB_H * 4];

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.1f)
            dt = 0.1f;

        Input in;
        int pause_pressed = 0, quit_pressed = 0;
        poll_input(&in, &pause_pressed, &quit_pressed);
        if (quit_pressed)
            break;
        if (pause_pressed) {
            in.anykey = 1;
            if (g->phase == ST_PLAYING) {
                g->phase = ST_PAUSED;
                game_msg(g, "Paused. P to resume.");
            } else if (g->phase == ST_PAUSED) {
                g->phase = ST_PLAYING;
            }
        }
        if (g->phase == ST_PAUSED)
            memset(&in, 0, sizeof(in));
        if ((g->phase == ST_DEAD || g->phase == ST_WIN) &&
            (IsKeyPressed(KEY_R) || IsKeyPressed(KEY_ENTER))) {
            int lvl = g->level, kills = g->kills;
            game_init(g, lvl);
            g->phase = ST_PLAYING;
            g->kills = kills;
            memset(&in, 0, sizeof(in));
        }
        game_update(g, &in, dt);

        int n = game_sprites(g, spr);
        render_frame(pixels, zbuf, g->px, g->py, g->angle, g->level,
                     game_door_open, g, spr, n, g->weapon, g->muzzle);
        if (g->hurt_fx > 0.0f) {
            for (int i = 0; i < FB_W * FB_H; i += 7) {
                uint32_t c = pixels[i];
                unsigned r = (c >> 16) & 255u;
                r += (unsigned)((255 - r) * g->hurt_fx * 0.4f);
                pixels[i] = 0xFF000000u | (r << 16) | (c & 0x00FFFFu);
            }
        }
        for (int i = 0; i < FB_W * FB_H; i++) {
            uint32_t c = pixels[i];
            rgba[4 * i + 0] = (unsigned char)((c >> 16) & 255u);
            rgba[4 * i + 1] = (unsigned char)((c >> 8) & 255u);
            rgba[4 * i + 2] = (unsigned char)(c & 255u);
            rgba[4 * i + 3] = 255;
        }
        UpdateTexture(tex, rgba);

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(tex,
            (Rectangle){ 0, 0, (float)FB_W, (float)FB_H },
            (Rectangle){ 0, 0, (float)view_w, (float)view_h },
            (Vector2){ 0, 0 }, 0.0f, WHITE);
        DrawRectangle(0, view_h, view_w, HUD_H, DARKGRAY);
        draw_hud(g, view_h);
        if (g->phase == ST_TITLE)
            draw_center_box("DOOMC", "Press any key to play", view_h, view_w);
        else if (g->phase == ST_PAUSED)
            draw_center_box("PAUSED", "P to resume, Q to quit", view_h, view_w);
        else if (g->phase == ST_DEAD)
            draw_center_box("YOU DIED", "R restart, Q quit", view_h, view_w);
        else if (g->phase == ST_WIN)
            draw_center_box("YOU ESCAPED!", "R replay, Q quit", view_h, view_w);
        EndDrawing();

        char title[128];
        int alive = 0;
        for (int i = 0; i < g->nenemies; i++)
            if (g->enemies[i].alive)
                alive++;
        snprintf(title, sizeof(title), "doomc L%d HP:%d AMMO:%d %s FOES:%d %s",
                 g->level + 1, g->hp, g->ammo,
                 g->weapon ? "SHOTGUN" : "PISTOL", alive, g->msg);
        SetWindowTitle(title);
    }
    UnloadTexture(tex);
    CloseWindow();
    return 0;
}

int main(int argc, char *argv[])
{
    int level = 0;
    const char *prog = argc > 0 ? argv[0] : "doomc";
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            usage(prog);
            return 0;
        } else if (strcmp(argv[i], "--level") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "--level needs an argument 1-%d\n", MAP_COUNT);
                return 1;
            }
            level = atoi(argv[i]) - 1;
        } else if (argv[i][0] != '-') {
            level = atoi(argv[i]) - 1;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            usage(prog);
            return 1;
        }
    }
    if (level < 0 || level >= MAP_COUNT) {
        fprintf(stderr, "Level must be 1-%d (got %d)\n", MAP_COUNT, level + 1);
        return 1;
    }

    srand((unsigned)time(NULL));
    tex_init();

    Game g;
    game_init(&g, level);
    return ray_loop(&g);
}
