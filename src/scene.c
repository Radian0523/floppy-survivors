#include "game.h"
#include <math.h>
#include <stdio.h>

void scene_title_update(GameState *gs, float dt) {
    gs->scene_timer += dt;

    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) ||
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        gs->scene = SCENE_GAME;
        gs->scene_timer = 0;
    }
}

void scene_title_draw(const GameState *gs) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    float pulse = 0.7f + 0.3f * sinf(gs->scene_timer * 2.0f);

    const char *title = "DISK SURVIVOR";
    int title_size = 64;
    int tw = MeasureText(title, title_size);
    int tx = (sw - tw) / 2;
    int ty = sh / 2 - 100;

    DrawText(title, tx + 3, ty + 3, title_size, (Color){255, 0, 100, 100});
    DrawText(title, tx - 3, ty - 3, title_size, (Color){0, 255, 255, 100});
    DrawText(title, tx, ty, title_size,
        (Color){255, 255, 255, (unsigned char)(255 * pulse)});

    const char *subtitle = "Survive 5 minutes against corrupted data";
    int sub_size = 16;
    int sw_text = MeasureText(subtitle, sub_size);
    DrawText(subtitle, (sw - sw_text) / 2, ty + title_size + 10, sub_size,
        (Color){180, 180, 200, 200});

    float blink = sinf(gs->scene_timer * 3.0f);
    if (blink > -0.3f) {
        const char *prompt = "PRESS SPACE or CLICK to START";
        int p_size = 20;
        int pw = MeasureText(prompt, p_size);
        DrawText(prompt, (sw - pw) / 2, sh / 2 + 60, p_size,
            (Color){100, 255, 255, 255});
    }

    const char *controls = "WASD/Arrows: Move  -  Mouse: Direction  -  Auto-attack";
    int c_size = 14;
    int cw = MeasureText(controls, c_size);
    DrawText(controls, (sw - cw) / 2, sh - 60, c_size, (Color){150, 150, 170, 200});

    const char *credit = "1.44MB GAME_DEV CONTEST 2026";
    int cr_size = 12;
    int crw = MeasureText(credit, cr_size);
    DrawText(credit, (sw - crw) / 2, sh - 30, cr_size, (Color){100, 100, 120, 200});
}

void scene_result_update(GameState *gs, float dt) {
    gs->scene_timer += dt;

    if (gs->scene_timer < 0.5f) return;

    if (IsKeyPressed(KEY_R) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) ||
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        gs->scene = SCENE_TITLE;
        gs->scene_timer = 0;
    }
}

void scene_result_draw(const GameState *gs) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, sh, (Color){0, 0, 0, 200});

    const char *msg = gs->victory ? "DISK RECOVERED!" : "DATA CORRUPTED";
    Color msg_color = gs->victory
        ? (Color){100, 255, 100, 255}
        : (Color){255, 100, 100, 255};
    int msg_size = 48;
    int mw = MeasureText(msg, msg_size);
    DrawText(msg, (sw - mw) / 2, sh / 2 - 140, msg_size, msg_color);

    char buf[64];
    int line_size = 22;
    int line_y = sh / 2 - 60;
    int line_gap = 32;

    int minutes = (int)gs->game_time / 60;
    int seconds = (int)gs->game_time % 60;
    sprintf(buf, "TIME      %d:%02d", minutes, seconds);
    int bw = MeasureText(buf, line_size);
    DrawText(buf, (sw - bw) / 2, line_y, line_size, (Color){200, 200, 220, 255});

    sprintf(buf, "LEVEL     %d", gs->level);
    bw = MeasureText(buf, line_size);
    DrawText(buf, (sw - bw) / 2, line_y + line_gap, line_size, (Color){100, 255, 255, 255});

    sprintf(buf, "KILLS     %d", gs->kills);
    bw = MeasureText(buf, line_size);
    DrawText(buf, (sw - bw) / 2, line_y + line_gap * 2, line_size, (Color){255, 200, 100, 255});

    if (gs->boss_defeated) {
        const char *boss_msg = "* FORMAT DEFEATED *";
        int bms = 18;
        int bmw = MeasureText(boss_msg, bms);
        DrawText(boss_msg, (sw - bmw) / 2, line_y + line_gap * 3 + 10, bms,
            (Color){255, 200, 50, 255});
    }

    if (gs->scene_timer >= 0.5f) {
        float blink = sinf(gs->scene_timer * 3.0f);
        if (blink > -0.3f) {
            const char *prompt = "PRESS SPACE to return to TITLE";
            int ps = 16;
            int pw = MeasureText(prompt, ps);
            DrawText(prompt, (sw - pw) / 2, sh - 80, ps, (Color){180, 180, 200, 255});
        }
    }
}
