#include "game.h"
#include <stdio.h>

typedef struct {
    int key;
    const char *label;
} DebugAction;

static const DebugAction actions[] = {
    {KEY_RIGHT_BRACKET, "]   : +30s time skip"},
    {KEY_LEFT_BRACKET,  "[   : -30s time skip"},
    {KEY_B,             "B   : spawn boss FORMAT"},
    {KEY_E,             "E   : spawn elite"},
    {KEY_F,             "F   : spawn formation"},
    {KEY_K,             "K   : kill all enemies"},
    {KEY_L,             "L   : force level up"},
    {KEY_H,             "H   : restore HP"},
    {KEY_I,             "I   : toggle invincibility"},
    {KEY_C,             "C   : drop chest at player"},
    {KEY_M,             "M   : drop magnet at player"},
    {KEY_J,             "J   : drop HP item at player"},
    {KEY_P,             "P   : toggle auto-play (bot)"},
};

void debug_update(GameState *gs) {
    if (IsKeyPressed(KEY_TAB)) gs->debug_open = !gs->debug_open;
    if (!gs->debug_open) return;

    if (IsKeyPressed(KEY_RIGHT_BRACKET)) {
        gs->game_time += 30.0f;
        if (gs->game_time > GAME_DURATION) gs->game_time = GAME_DURATION;
    }
    if (IsKeyPressed(KEY_LEFT_BRACKET)) {
        gs->game_time -= 30.0f;
        if (gs->game_time < 0) gs->game_time = 0;
    }
    if (IsKeyPressed(KEY_B)) {
        if (!gs->boss.active && !gs->boss_defeated) boss_spawn(gs);
    }
    if (IsKeyPressed(KEY_E)) {
        enemy_spawn_elite_force(gs);
    }
    if (IsKeyPressed(KEY_F)) {
        enemy_spawn_formation_force(gs);
    }
    if (IsKeyPressed(KEY_K)) {
        for (int i = 0; i < MAX_ENEMIES; i++) gs->enemies[i].active = false;
    }
    if (IsKeyPressed(KEY_L)) {
        gs->xp = gs->xp_to_next;
    }
    if (IsKeyPressed(KEY_H)) {
        gs->player.hp = gs->player.max_hp;
    }
    if (IsKeyPressed(KEY_I)) {
        gs->debug_invincible = !gs->debug_invincible;
    }
    if (IsKeyPressed(KEY_C)) {
        chest_drop(gs, gs->player.pos);
    }
    if (IsKeyPressed(KEY_M)) {
        // Find inactive item slot and spawn magnet
        for (int i = 0; i < MAX_ITEMS; i++) {
            if (!gs->items[i].active) {
                gs->items[i].active = true;
                gs->items[i].pos = gs->player.pos;
                gs->items[i].type = ITEM_MAGNET;
                gs->items[i].life = ITEM_LIFE;
                break;
            }
        }
    }
    if (IsKeyPressed(KEY_J)) {
        for (int i = 0; i < MAX_ITEMS; i++) {
            if (!gs->items[i].active) {
                gs->items[i].active = true;
                gs->items[i].pos = gs->player.pos;
                gs->items[i].type = ITEM_HP;
                gs->items[i].life = ITEM_LIFE;
                break;
            }
        }
    }
    if (IsKeyPressed(KEY_P)) {
        gs->bot_mode = !gs->bot_mode;
    }
}

void debug_draw(const GameState *gs) {
    if (!gs->debug_open) return;

    int sw = GetScreenWidth();
    int n = (int)(sizeof(actions) / sizeof(actions[0]));
    int line_h = 16;
    int pad = 8;
    int w = 260;
    int h = n * line_h + 80;
    int x = sw - w - 10;
    int y = 10;

    DrawRectangle(x, y, w, h, (Color){10, 15, 25, 220});
    DrawRectangleLines(x, y, w, h, (Color){100, 200, 255, 200});

    DrawText("DEBUG  (TAB to close)", x + pad, y + pad, 14,
        (Color){100, 200, 255, 255});

    char info[80];
    int active = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) if (gs->enemies[i].active) active++;
    int mins = (int)gs->game_time / 60;
    int secs = (int)gs->game_time % 60;
    sprintf(info, "T %d:%02d  E %d  FPS %d  INV %s  BOT %s",
        mins, secs, active, GetFPS(),
        gs->debug_invincible ? "ON" : "off",
        gs->bot_mode ? "ON" : "off");
    DrawText(info, x + pad, y + pad + 22, 12, (Color){200, 220, 240, 255});

    int row_y = y + pad + 50;
    for (int i = 0; i < n; i++) {
        DrawText(actions[i].label, x + pad, row_y, 12, (Color){220, 220, 240, 255});
        row_y += line_h;
    }
}
