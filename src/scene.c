#include "game.h"
#include <math.h>
#include <stdio.h>

void scene_title_update(GameState *gs, float dt) {
    gs->scene_timer += dt;

    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) ||
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        gs->scene = SCENE_WEAPON_SELECT;
        gs->scene_timer = 0;
        gs->weapon_select_hover = 0;
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

static const char *weapon_names[STARTING_WEAPON_COUNT] = {
    "PULSE BOLT", "ORBITERS", "BEAM", "NOVA"
};

static const char *weapon_descs[STARTING_WEAPON_COUNT] = {
    "Auto-fire nearest enemy",
    "Rotating shields",
    "Sweeping laser",
    "Pulse wave"
};

static Color weapon_colors[STARTING_WEAPON_COUNT] = {
    {255, 255, 100, 255},   // Pulse: yellow
    {100, 150, 255, 255},   // Orbiters: blue
    {180, 80, 255, 255},    // Beam: violet
    {100, 255, 255, 255}    // Nova: cyan
};

static void draw_weapon_preview(int idx, float cx, float cy, float t) {
    Color col = weapon_colors[idx];
    switch (idx) {
        case STARTING_WEAPON_PULSE: {
            DrawCircleV((Vector2){cx, cy}, 8, col);
            DrawCircleV((Vector2){cx, cy}, 4, (Color){255, 255, 200, 255});
            float off = fmodf(t * 60.0f, 50.0f);
            DrawCircleV((Vector2){cx + off, cy - 20}, 4, col);
            DrawCircleV((Vector2){cx + off, cy + 20}, 4, col);
            break;
        }
        case STARTING_WEAPON_ORBITERS: {
            DrawCircleV((Vector2){cx, cy}, 8, (Color){0, 255, 255, 255});
            for (int i = 0; i < 3; i++) {
                float a = t * 2.0f + (2.0f * 3.14159f * i / 3.0f);
                float ox = cx + cosf(a) * 25;
                float oy = cy + sinf(a) * 25;
                DrawCircleV((Vector2){ox, oy}, 6, col);
            }
            break;
        }
        case STARTING_WEAPON_BEAM: {
            DrawCircleV((Vector2){cx, cy}, 8, (Color){0, 255, 255, 255});
            float a = sinf(t * 2.0f) * 0.7f;
            float ex = cx + cosf(a) * 50;
            float ey = cy + sinf(a) * 50;
            DrawLineEx((Vector2){cx, cy}, (Vector2){ex, ey}, 4, col);
            break;
        }
        case STARTING_WEAPON_NOVA: {
            DrawCircleV((Vector2){cx, cy}, 8, (Color){0, 255, 255, 255});
            float r = fmodf(t * 40.0f, 50.0f);
            float alpha = 1.0f - r / 50.0f;
            Color rc = {col.r, col.g, col.b, (unsigned char)(255 * alpha)};
            DrawRing((Vector2){cx, cy}, r - 2, r + 2, 0, 360, 24, rc);
            break;
        }
    }
}

void scene_weapon_select_update(GameState *gs, float dt) {
    gs->scene_timer += dt;

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int box_w = 160;
    int box_h = 200;
    int gap = 16;
    int total_w = STARTING_WEAPON_COUNT * box_w + (STARTING_WEAPON_COUNT - 1) * gap;
    int start_x = (sw - total_w) / 2;
    int start_y = sh / 2 - box_h / 2;

    Vector2 mouse = GetMousePosition();
    static Vector2 prev_mouse = {0, 0};
    bool mouse_moved = (mouse.x != prev_mouse.x || mouse.y != prev_mouse.y);
    prev_mouse = mouse;

    if (mouse_moved) {
        int hovered = -1;
        for (int i = 0; i < STARTING_WEAPON_COUNT; i++) {
            int x = start_x + i * (box_w + gap);
            if (mouse.x >= x && mouse.x < x + box_w &&
                mouse.y >= start_y && mouse.y < start_y + box_h) {
                hovered = i;
            }
        }
        if (hovered >= 0) gs->weapon_select_hover = hovered;
    }

    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        if (gs->weapon_select_hover <= 0) gs->weapon_select_hover = STARTING_WEAPON_COUNT - 1;
        else gs->weapon_select_hover--;
    }
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        if (gs->weapon_select_hover < 0) gs->weapon_select_hover = 0;
        else gs->weapon_select_hover = (gs->weapon_select_hover + 1) % STARTING_WEAPON_COUNT;
    }

    int selected = -1;
    if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) selected = 0;
    if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) selected = 1;
    if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) selected = 2;
    if (IsKeyPressed(KEY_FOUR) || IsKeyPressed(KEY_KP_4)) selected = 3;

    if ((IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) && gs->weapon_select_hover >= 0) {
        selected = gs->weapon_select_hover;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && gs->weapon_select_hover >= 0) {
        int x = start_x + gs->weapon_select_hover * (box_w + gap);
        if (mouse.x >= x && mouse.x < x + box_w &&
            mouse.y >= start_y && mouse.y < start_y + box_h) {
            selected = gs->weapon_select_hover;
        }
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        gs->scene = SCENE_TITLE;
        gs->scene_timer = 0;
        return;
    }

    if (selected >= 0 && selected < STARTING_WEAPON_COUNT) {
        game_start_with_weapon(gs, selected);
        gs->scene = SCENE_GAME;
        gs->scene_timer = 0;
    }
}

void scene_weapon_select_draw(const GameState *gs) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    const char *title = "CHOOSE YOUR WEAPON";
    int title_size = 32;
    int tw = MeasureText(title, title_size);
    DrawText(title, (sw - tw) / 2, sh / 2 - 180, title_size,
        (Color){100, 255, 255, 255});

    const char *hint = "Arrow/WASD: move  -  SPACE/Enter: confirm  -  1-4: quick select";
    int hs = 14;
    int hw = MeasureText(hint, hs);
    DrawText(hint, (sw - hw) / 2, sh / 2 - 140, hs, (Color){180, 180, 200, 200});

    int box_w = 160;
    int box_h = 200;
    int gap = 16;
    int total_w = STARTING_WEAPON_COUNT * box_w + (STARTING_WEAPON_COUNT - 1) * gap;
    int start_x = (sw - total_w) / 2;
    int start_y = sh / 2 - box_h / 2;

    for (int i = 0; i < STARTING_WEAPON_COUNT; i++) {
        int x = start_x + i * (box_w + gap);
        int y = start_y;
        bool hover = (gs->weapon_select_hover == i);
        Color col = weapon_colors[i];

        if (hover) {
            DrawRectangle(x - 3, y - 3, box_w + 6, box_h + 6, col);
        }
        DrawRectangle(x, y, box_w, box_h, (Color){15, 15, 25, 255});
        DrawRectangleLines(x, y, box_w, box_h, col);

        char num[4];
        sprintf(num, "%d", i + 1);
        DrawText(num, x + 8, y + 8, 16, (Color){150, 150, 150, 255});

        const char *name = weapon_names[i];
        int ns = 20;
        int nw = MeasureText(name, ns);
        DrawText(name, x + (box_w - nw) / 2, y + 18, ns, col);

        draw_weapon_preview(i, x + box_w / 2, y + 110, gs->scene_timer);

        const char *desc = weapon_descs[i];
        int ds = 12;
        int dw = MeasureText(desc, ds);
        DrawText(desc, x + (box_w - dw) / 2, y + box_h - 24, ds,
            (Color){180, 180, 200, 255});
    }

    const char *back = "ESC to return to title";
    int bs = 12;
    int bw = MeasureText(back, bs);
    DrawText(back, (sw - bw) / 2, sh - 30, bs, (Color){120, 120, 140, 200});
}

void pause_draw(const GameState *gs) {
    (void)gs;
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, sh, (Color){0, 0, 0, 180});

    const char *title = "PAUSED";
    int title_size = 56;
    int tw = MeasureText(title, title_size);
    DrawText(title, (sw - tw) / 2, sh / 2 - 80, title_size,
        (Color){100, 255, 255, 255});

    const char *line1 = "ESC / SPACE  -  Resume";
    int s1 = 18;
    int w1 = MeasureText(line1, s1);
    DrawText(line1, (sw - w1) / 2, sh / 2 + 10, s1, (Color){220, 220, 240, 255});

    const char *line2 = "Q  -  Return to Title";
    int s2 = 18;
    int w2 = MeasureText(line2, s2);
    DrawText(line2, (sw - w2) / 2, sh / 2 + 40, s2, (Color){220, 220, 240, 255});
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
