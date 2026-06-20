#include "game.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// ===== Title background drift entities =====
typedef enum {
    DRIFT_ENEMY_CIRCLE,
    DRIFT_ENEMY_RECT,
    DRIFT_ENEMY_TRI,
    DRIFT_GEM,
    DRIFT_BULLET,
    DRIFT_NOVA,
} DriftType;

typedef struct {
    Vector2 pos;
    Vector2 vel;
    float radius;
    float life;
    float max_life;
    DriftType type;
    Color color;
    bool active;
} TitleDrift;

#define TITLE_DRIFT_MAX 36
static TitleDrift drifts[TITLE_DRIFT_MAX];
static float drift_spawn_timer = 0;
static bool drifts_initialized = false;

static Color random_enemy_color(void) {
    Color cands[] = {
        {255, 50, 100, 255},
        {255, 180, 60, 255},
        {255, 120, 40, 255},
        {220, 80, 180, 255},
        {255, 80, 60, 255},
        {255, 60, 140, 255},
        {255, 100, 180, 255},
        {200, 60, 60, 255},
        {255, 90, 70, 255},
    };
    return cands[rand() % (int)(sizeof(cands) / sizeof(cands[0]))];
}

static void title_drift_init(void) {
    for (int i = 0; i < TITLE_DRIFT_MAX; i++) drifts[i].active = false;
    drift_spawn_timer = 0;
    drifts_initialized = true;
}

static void spawn_drift(int sw, int sh) {
    for (int i = 0; i < TITLE_DRIFT_MAX; i++) {
        if (drifts[i].active) continue;
        TitleDrift *d = &drifts[i];
        d->active = true;

        // Random edge entry
        int side = rand() % 4;
        switch (side) {
            case 0: d->pos.x = rand() % sw; d->pos.y = -30; break;
            case 1: d->pos.x = rand() % sw; d->pos.y = sh + 30; break;
            case 2: d->pos.x = -30; d->pos.y = rand() % sh; break;
            default: d->pos.x = sw + 30; d->pos.y = rand() % sh; break;
        }

        float angle = ((rand() % 360) / 180.0f) * 3.14159f;
        float speed = 18.0f + (rand() % 30);
        d->vel.x = cosf(angle) * speed;
        d->vel.y = sinf(angle) * speed;
        d->life = 8.0f + (rand() % 40) * 0.1f;
        d->max_life = d->life;

        int kind = rand() % 100;
        if (kind < 50) {
            int shape = rand() % 3;
            d->type = (DriftType)(DRIFT_ENEMY_CIRCLE + shape);
            d->radius = 8 + (rand() % 8);
            d->color = random_enemy_color();
        } else if (kind < 70) {
            d->type = DRIFT_GEM;
            d->radius = 6;
            d->color = (Color){120, 255, 120, 255};
        } else if (kind < 90) {
            d->type = DRIFT_BULLET;
            d->radius = 4;
            d->color = (Color){255, 255, 100, 255};
            float bspeed = 120 + (rand() % 60);
            d->vel.x = cosf(angle) * bspeed;
            d->vel.y = sinf(angle) * bspeed;
            d->life = 4.0f;
            d->max_life = 4.0f;
        } else {
            d->type = DRIFT_NOVA;
            d->radius = 0;
            d->pos.x = rand() % sw;
            d->pos.y = rand() % sh;
            d->vel.x = 0;
            d->vel.y = 0;
            d->color = (Color){100, 255, 255, 255};
            d->life = 1.5f;
            d->max_life = 1.5f;
        }
        return;
    }
}

static void title_drift_update(float dt, int sw, int sh) {
    if (!drifts_initialized) title_drift_init();

    drift_spawn_timer -= dt;
    if (drift_spawn_timer <= 0) {
        drift_spawn_timer = 0.25f + (rand() % 30) * 0.01f;
        spawn_drift(sw, sh);
    }

    for (int i = 0; i < TITLE_DRIFT_MAX; i++) {
        if (!drifts[i].active) continue;
        TitleDrift *d = &drifts[i];
        d->pos.x += d->vel.x * dt;
        d->pos.y += d->vel.y * dt;
        d->life -= dt;
        if (d->type == DRIFT_NOVA) {
            d->radius += 120.0f * dt;
        }
        if (d->life <= 0 ||
            d->pos.x < -50 || d->pos.x > sw + 50 ||
            d->pos.y < -50 || d->pos.y > sh + 50) {
            d->active = false;
        }
    }
}

static void title_drift_draw_internal(void);

void scene_title_draw_world(void) {
    title_drift_draw_internal();
}

static void title_drift_draw_internal(void) {
    float t = (float)GetTime();
    for (int i = 0; i < TITLE_DRIFT_MAX; i++) {
        if (!drifts[i].active) continue;
        const TitleDrift *d = &drifts[i];
        float ratio = d->life / d->max_life;
        float alpha = ratio;
        if (ratio > 0.85f) alpha = (1.0f - ratio) / 0.15f;
        if (alpha > 1) alpha = 1;
        if (alpha < 0) alpha = 0;
        Color c = d->color;
        c.a = (unsigned char)(c.a * alpha * 0.55f);
        Color inner = {255, 255, 255, (unsigned char)(255 * alpha * 0.3f)};

        switch (d->type) {
            case DRIFT_ENEMY_CIRCLE:
                DrawCircleV(d->pos, d->radius, c);
                DrawCircleV(d->pos, d->radius * 0.5f, inner);
                break;
            case DRIFT_ENEMY_RECT:
                DrawRectangle((int)(d->pos.x - d->radius),
                              (int)(d->pos.y - d->radius),
                              (int)(d->radius * 2), (int)(d->radius * 2), c);
                break;
            case DRIFT_ENEMY_TRI: {
                float a = atan2f(d->vel.y, d->vel.x);
                Vector2 tip = {d->pos.x + cosf(a) * d->radius,
                               d->pos.y + sinf(a) * d->radius};
                Vector2 bl = {d->pos.x + cosf(a + 2.4f) * d->radius * 0.8f,
                              d->pos.y + sinf(a + 2.4f) * d->radius * 0.8f};
                Vector2 br = {d->pos.x + cosf(a - 2.4f) * d->radius * 0.8f,
                              d->pos.y + sinf(a - 2.4f) * d->radius * 0.8f};
                DrawTriangle(tip, bl, br, c);
                DrawTriangle(tip, br, bl, c);
                break;
            }
            case DRIFT_GEM: {
                float pulse = 1.0f + 0.15f * sinf(t * 3.0f + i);
                float rr = d->radius * pulse;
                Vector2 top    = {d->pos.x, d->pos.y - rr * 1.2f};
                Vector2 right  = {d->pos.x + rr * 0.9f, d->pos.y};
                Vector2 bottom = {d->pos.x, d->pos.y + rr * 1.2f};
                Vector2 left   = {d->pos.x - rr * 0.9f, d->pos.y};
                DrawTriangle(top, left, right, c);
                DrawTriangle(bottom, right, left, c);
                break;
            }
            case DRIFT_BULLET: {
                float vlen = sqrtf(d->vel.x * d->vel.x + d->vel.y * d->vel.y);
                if (vlen < 0.01f) vlen = 1;
                float dx = d->vel.x / vlen;
                float dy = d->vel.y / vlen;
                Vector2 head = {d->pos.x + dx * d->radius,
                                d->pos.y + dy * d->radius};
                Vector2 tail = {d->pos.x - dx * d->radius * 4.0f,
                                d->pos.y - dy * d->radius * 4.0f};
                DrawLineEx(tail, head, d->radius * 1.5f, c);
                break;
            }
            case DRIFT_NOVA: {
                if (d->radius > 0) {
                    DrawRing(d->pos, d->radius - 3, d->radius + 3,
                             0, 360, 36, c);
                }
                break;
            }
        }
    }
}

typedef struct {
    const char *name;
    int value;
    Color color;
} DifficultyPreset;

static const DifficultyPreset difficulties[] = {
    {"EASY",   DIFFICULTY_EASY,   {120, 255, 180, 255}},
    {"NORMAL", DIFFICULTY_NORMAL, {120, 200, 255, 255}},
    {"HARD",   DIFFICULTY_HARD,   {255, 180, 100, 255}},
    {"BRUTAL", DIFFICULTY_BRUTAL, {255, 100, 120, 255}},
};
#define DIFFICULTY_COUNT 4

static int find_difficulty_index(int value) {
    int best = 1;
    int min_diff = 1000;
    for (int i = 0; i < DIFFICULTY_COUNT; i++) {
        int d = difficulties[i].value - value;
        if (d < 0) d = -d;
        if (d < min_diff) { min_diff = d; best = i; }
    }
    return best;
}

void scene_title_update(GameState *gs, float dt) {
    gs->scene_timer += dt;
    title_drift_update(dt, GetScreenWidth(), GetScreenHeight());

    int idx = find_difficulty_index(gs->difficulty);
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        idx = (idx - 1 + DIFFICULTY_COUNT) % DIFFICULTY_COUNT;
        gs->difficulty = difficulties[idx].value;
    }
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        idx = (idx + 1) % DIFFICULTY_COUNT;
        gs->difficulty = difficulties[idx].value;
    }

    if (IsKeyPressed(KEY_H)) {
        gs->scene = SCENE_HOW_TO_PLAY;
        gs->scene_timer = 0;
        return;
    }
    if (IsKeyPressed(KEY_S)) {
        gs->scene = SCENE_SETTINGS;
        gs->scene_timer = 0;
        gs->settings_hover = 0;
        return;
    }

    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) ||
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        params_from_difficulty(&g_params, (float)gs->difficulty);
        gs->scene = SCENE_WEAPON_SELECT;
        gs->scene_timer = 0;
        gs->weapon_select_hover = 0;
    }
}

// ============== HOW TO PLAY ==============

void scene_how_to_play_update(GameState *gs, float dt) {
    gs->scene_timer += dt;
    title_drift_update(dt, GetScreenWidth(), GetScreenHeight());

    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_H) ||
        IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) ||
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        gs->scene = SCENE_TITLE;
        gs->scene_timer = 0;
    }
}

void scene_how_to_play_draw(const GameState *gs) {
    (void)gs;
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, sh, (Color){0, 0, 10, 180});

    const char *title = "HOW TO PLAY";
    int ts = 36;
    int tw = MeasureText(title, ts);
    DrawText(title, (sw - tw) / 2, 40, ts, (Color){100, 255, 255, 255});

    int left = sw / 2 - 280;
    int y = 110;
    int line = 22;

    Color section = {255, 230, 100, 255};
    Color label   = {220, 220, 240, 255};
    Color val     = {180, 200, 220, 255};
    Color accent_e = {255, 100, 120, 255};   // enemy
    Color accent_f = {120, 255, 180, 255};   // friendly

    DrawText("CONTROLS", left, y, 18, section); y += line + 2;
    DrawText("  Move:        WASD / Arrow keys / Mouse hold", left, y, 16, label); y += line;
    DrawText("  Confirm:     SPACE / Enter / Click", left, y, 16, label); y += line;
    DrawText("  Pause:       ESC  (during game)", left, y, 16, label); y += line;
    DrawText("  Debug:       TAB  (during game)", left, y, 16, val); y += line + 8;

    DrawText("OBJECTIVE", left, y, 18, section); y += line + 2;
    DrawText("  Survive 5 minutes against corrupted data.", left, y, 16, label); y += line;
    DrawText("  Weapons fire automatically. You only move.", left, y, 16, label); y += line + 8;

    DrawText("GAMEPLAY", left, y, 18, section); y += line + 2;
    DrawText("  - Collect green GEMS for XP", left, y, 16, accent_f); y += line;
    DrawText("  - LEVEL UP to choose an upgrade or new weapon", left, y, 16, label); y += line;
    DrawText("  - Defeat ELITES (with crowns) for treasure CHESTS", left, y, 16, accent_e); y += line;
    DrawText("  - Open chests for guaranteed upgrades", left, y, 16, label); y += line;
    DrawText("  - Survive the FORMAT boss at 4:00", left, y, 16, accent_e); y += line + 8;

    DrawText("ITEMS", left, y, 18, section); y += line + 2;
    DrawText("  HP cross   - restore 2 HP", left, y, 16, accent_f); y += line;
    DrawText("  Big gem    - magnet, pulls all gems to you", left, y, 16, accent_f); y += line;

    const char *back = "Press ESC / SPACE / H to return to title";
    int bw = MeasureText(back, 14);
    DrawText(back, (sw - bw) / 2, sh - 40, 14, (Color){150, 200, 220, 255});
}

// ============== SETTINGS ==============

#define SETTINGS_ROW_COUNT 3
#define SETTINGS_ROW_BGM 0
#define SETTINGS_ROW_SFX 1
#define SETTINGS_ROW_FS  2

void scene_settings_update(GameState *gs, float dt) {
    gs->scene_timer += dt;
    title_drift_update(dt, GetScreenWidth(), GetScreenHeight());

    if (IsKeyPressed(KEY_ESCAPE)) {
        settings_save(&gs->settings);
        gs->scene = SCENE_TITLE;
        gs->scene_timer = 0;
        return;
    }

    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        gs->settings_hover = (gs->settings_hover - 1 + SETTINGS_ROW_COUNT) % SETTINGS_ROW_COUNT;
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        gs->settings_hover = (gs->settings_hover + 1) % SETTINGS_ROW_COUNT;
    }

    bool changed = false;
    float step = 0.1f;
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        if (gs->settings_hover == SETTINGS_ROW_BGM) {
            gs->settings.bgm_volume -= step;
            if (gs->settings.bgm_volume < 0) gs->settings.bgm_volume = 0;
            changed = true;
        } else if (gs->settings_hover == SETTINGS_ROW_SFX) {
            gs->settings.sfx_volume -= step;
            if (gs->settings.sfx_volume < 0) gs->settings.sfx_volume = 0;
            changed = true;
        } else if (gs->settings_hover == SETTINGS_ROW_FS) {
            gs->settings.fullscreen = !gs->settings.fullscreen;
            changed = true;
        }
    }
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        if (gs->settings_hover == SETTINGS_ROW_BGM) {
            gs->settings.bgm_volume += step;
            if (gs->settings.bgm_volume > 1) gs->settings.bgm_volume = 1;
            changed = true;
        } else if (gs->settings_hover == SETTINGS_ROW_SFX) {
            gs->settings.sfx_volume += step;
            if (gs->settings.sfx_volume > 1) gs->settings.sfx_volume = 1;
            changed = true;
        } else if (gs->settings_hover == SETTINGS_ROW_FS) {
            gs->settings.fullscreen = !gs->settings.fullscreen;
            changed = true;
        }
    }
    if ((IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) &&
        gs->settings_hover == SETTINGS_ROW_FS) {
        gs->settings.fullscreen = !gs->settings.fullscreen;
        changed = true;
    }

    if (changed) settings_apply(&gs->settings);
}

static void draw_setting_row(int x, int y, int w, const char *label,
                              const char *value_text, float value_ratio,
                              bool selected) {
    Color c_label = selected ? (Color){255, 255, 255, 255}
                              : (Color){180, 180, 200, 255};
    Color c_val   = selected ? (Color){100, 255, 255, 255}
                              : (Color){180, 200, 220, 255};
    Color c_bg    = selected ? (Color){25, 30, 50, 230}
                              : (Color){15, 18, 28, 200};
    Color c_border = selected ? (Color){100, 255, 255, 255}
                               : (Color){60, 80, 100, 200};

    int h = 40;
    DrawRectangle(x, y, w, h, c_bg);
    DrawRectangleLines(x, y, w, h, c_border);
    DrawText(label, x + 16, y + 12, 16, c_label);

    int bar_x = x + 220;
    int bar_w = w - 240;
    if (value_ratio >= 0) {
        int bar_y = y + h / 2 - 4;
        int bh = 8;
        DrawRectangle(bar_x, bar_y, bar_w, bh, (Color){30, 30, 50, 255});
        DrawRectangle(bar_x, bar_y, (int)(bar_w * value_ratio), bh, c_val);
    }
    DrawText(value_text, bar_x + bar_w + 12, y + 12, 16, c_val);
}

void scene_settings_draw(const GameState *gs) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, sh, (Color){0, 0, 10, 180});

    const char *title = "SETTINGS";
    int ts = 36;
    int tw = MeasureText(title, ts);
    DrawText(title, (sw - tw) / 2, 60, ts, (Color){100, 255, 255, 255});

    int row_w = 520;
    int row_x = (sw - row_w) / 2;
    int row_y = 150;
    int row_gap = 12;

    char buf[32];
    float bgm = gs->settings.bgm_volume;
    float sfx = gs->settings.sfx_volume;

    snprintf(buf, sizeof(buf), "%d%%", (int)(bgm * 100 + 0.5f));
    draw_setting_row(row_x, row_y, row_w, "BGM VOLUME", buf, bgm,
                     gs->settings_hover == SETTINGS_ROW_BGM);
    row_y += 40 + row_gap;

    snprintf(buf, sizeof(buf), "%d%%", (int)(sfx * 100 + 0.5f));
    draw_setting_row(row_x, row_y, row_w, "SFX VOLUME", buf, sfx,
                     gs->settings_hover == SETTINGS_ROW_SFX);
    row_y += 40 + row_gap;

    const char *fs_val = gs->settings.fullscreen ? "ON" : "OFF";
    draw_setting_row(row_x, row_y, row_w, "FULLSCREEN", fs_val, -1.0f,
                     gs->settings_hover == SETTINGS_ROW_FS);

    const char *hint = "Up/Down: row  -  Left/Right: change  -  ESC: save & back";
    int hw = MeasureText(hint, 14);
    DrawText(hint, (sw - hw) / 2, sh - 60, 14, (Color){150, 200, 220, 255});
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

    // Difficulty selector
    int idx = find_difficulty_index(gs->difficulty);
    const char *diff_label = "DIFFICULTY";
    int dl_size = 14;
    int dl_w = MeasureText(diff_label, dl_size);
    DrawText(diff_label, (sw - dl_w) / 2, sh / 2 + 30, dl_size,
        (Color){160, 160, 180, 255});

    int box_w = 80;
    int gap = 12;
    int total_w = DIFFICULTY_COUNT * box_w + (DIFFICULTY_COUNT - 1) * gap;
    int start_x = (sw - total_w) / 2;
    int box_y = sh / 2 + 50;
    int box_h = 32;

    for (int i = 0; i < DIFFICULTY_COUNT; i++) {
        int x = start_x + i * (box_w + gap);
        Color col = difficulties[i].color;
        bool selected = (i == idx);
        if (selected) {
            DrawRectangle(x - 3, box_y - 3, box_w + 6, box_h + 6, col);
        }
        DrawRectangle(x, box_y, box_w, box_h, (Color){10, 15, 25, 230});
        DrawRectangleLines(x, box_y, box_w, box_h, col);
        const char *n = difficulties[i].name;
        int nw = MeasureText(n, 14);
        DrawText(n, x + (box_w - nw) / 2, box_y + 9, 14, col);
    }

    const char *diff_hint = "<- / -> or A / D to change";
    int dh = 11;
    int dhw = MeasureText(diff_hint, dh);
    DrawText(diff_hint, (sw - dhw) / 2, box_y + box_h + 8, dh,
        (Color){130, 130, 150, 200});

    float blink = sinf(gs->scene_timer * 3.0f);
    if (blink > -0.3f) {
        const char *prompt = "PRESS SPACE or CLICK to START";
        int p_size = 18;
        int pw = MeasureText(prompt, p_size);
        DrawText(prompt, (sw - pw) / 2, box_y + box_h + 32, p_size,
            (Color){100, 255, 255, 255});
    }

    const char *controls = "H: How to Play  -  S: Settings";
    int c_size = 14;
    int cw = MeasureText(controls, c_size);
    DrawText(controls, (sw - cw) / 2, sh - 60, c_size, (Color){150, 150, 170, 200});

    const char *credit = "1.44MB GAME_DEV CONTEST 2026";
    int cr_size = 12;
    int crw = MeasureText(credit, cr_size);
    DrawText(credit, (sw - crw) / 2, sh - 30, cr_size, (Color){100, 100, 120, 200});

    if (gs->best.total_games > 0) {
        char buf[96];
        int m = (int)gs->best.best_time / 60;
        int s = (int)gs->best.best_time % 60;
        sprintf(buf, "BEST  %d:%02d  -  KILLS %d  -  LV %d",
            m, s, gs->best.best_kills, gs->best.best_level);
        int bs = 14;
        int bw = MeasureText(buf, bs);
        DrawText(buf, (sw - bw) / 2, sh / 2 + 130, bs, (Color){180, 200, 220, 220});
        if (gs->best.boss_defeats > 0) {
            char bd[48];
            sprintf(bd, "FORMAT DEFEATED x%d", gs->best.boss_defeats);
            int bdw = MeasureText(bd, bs);
            DrawText(bd, (sw - bdw) / 2, sh / 2 + 152, bs, (Color){255, 200, 100, 220});
        }
    }
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

// Per-weapon row colors (matches in-game weapon hues)
static Color weapon_row_color(WeaponID w) {
    switch (w) {
        case WEAPON_ID_PULSE:     return (Color){255, 220, 100, 255};
        case WEAPON_ID_ORBITERS:  return (Color){120, 220, 255, 255};
        case WEAPON_ID_BEAM:      return (Color){180, 200, 255, 255};
        case WEAPON_ID_NOVA:      return (Color){150, 200, 255, 255};
        case WEAPON_ID_MINES:     return (Color){255, 220, 120, 255};
        case WEAPON_ID_CHAIN:     return (Color){200, 240, 255, 255};
        case WEAPON_ID_BOOMERANG: return (Color){150, 255, 180, 255};
        case WEAPON_ID_TRAIL:     return (Color){120, 220, 255, 255};
        case WEAPON_ID_WHIP:      return (Color){220, 220, 255, 255};
        default:                  return (Color){200, 200, 220, 255};
    }
}

void scene_result_draw(const GameState *gs) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, sh, (Color){0, 0, 0, 220});

    // === Header ===
    const char *msg = gs->victory ? "DISK RECOVERED!" : "DATA CORRUPTED";
    Color msg_color = gs->victory
        ? (Color){100, 255, 100, 255}
        : (Color){255, 100, 100, 255};
    int msg_size = 36;
    int mw = MeasureText(msg, msg_size);
    int top_y = 30;
    DrawText(msg, (sw - mw) / 2, top_y, msg_size, msg_color);

    // === Score badges ===
    float blink_t = sinf(gs->scene_timer * 5.0f);
    if (blink_t > -0.3f && gs->last_score_flags) {
        const char *labels[4] = {"NEW TIME", "NEW KILLS", "NEW LEVEL", "FIRST CLEAR"};
        Color colors[4] = {
            {100, 255, 200, 255},
            {255, 200, 100, 255},
            {100, 200, 255, 255},
            {255, 100, 255, 255}
        };
        int total_w = 0, count = 0, lw[4], li[4];
        for (int i = 0; i < 4; i++) {
            if (gs->last_score_flags & (1 << i)) {
                li[count] = i;
                lw[count] = MeasureText(labels[i], 14);
                total_w += lw[count];
                count++;
            }
        }
        total_w += (count - 1) * 14;
        int x = (sw - total_w) / 2;
        for (int i = 0; i < count; i++) {
            DrawText(labels[li[i]], x, top_y + msg_size + 6, 14, colors[li[i]]);
            x += lw[i] + 14;
        }
    }

    // === Summary stats (top row) ===
    char buf[96];
    int stat_y = top_y + msg_size + 32;
    int stat_size = 16;

    int total_dmg = 0;
    for (int i = 0; i < WEAPON_ID_COUNT; i++) total_dmg += gs->stats.damage_dealt[i];
    int minutes = (int)gs->game_time / 60;
    int seconds = (int)gs->game_time % 60;

    // Two-column compact summary
    int col_w = sw / 2 - 40;
    int col1_x = 40;
    int col2_x = sw / 2 + 20;
    int row_h = 22;

    Color label_col = (Color){140, 140, 160, 255};
    Color value_col = (Color){240, 240, 255, 255};

    // Helper: draw label left, value right-aligned within a column
    Color level_col  = (Color){100, 255, 255, 255};
    Color kills_col  = (Color){255, 200, 100, 255};
    Color dps_col    = (Color){255, 200, 100, 255};
    Color taken_col  = (Color){255, 130, 130, 255};
    Color gems_col   = (Color){150, 255, 180, 255};

    int vw;
    DrawText("TIME", col1_x, stat_y + 0 * row_h, stat_size, label_col);
    sprintf(buf, "%d:%02d", minutes, seconds);
    vw = MeasureText(buf, stat_size);
    DrawText(buf, col1_x + col_w - vw, stat_y + 0 * row_h, stat_size, value_col);

    DrawText("LEVEL", col1_x, stat_y + 1 * row_h, stat_size, label_col);
    sprintf(buf, "%d", gs->level);
    vw = MeasureText(buf, stat_size);
    DrawText(buf, col1_x + col_w - vw, stat_y + 1 * row_h, stat_size, level_col);

    DrawText("KILLS", col1_x, stat_y + 2 * row_h, stat_size, label_col);
    sprintf(buf, "%d", gs->kills);
    vw = MeasureText(buf, stat_size);
    DrawText(buf, col1_x + col_w - vw, stat_y + 2 * row_h, stat_size, kills_col);

    DrawText("TOTAL DMG", col2_x, stat_y + 0 * row_h, stat_size, label_col);
    sprintf(buf, "%d", total_dmg);
    vw = MeasureText(buf, stat_size);
    DrawText(buf, col2_x + col_w - vw, stat_y + 0 * row_h, stat_size, dps_col);

    DrawText("DAMAGE TAKEN", col2_x, stat_y + 1 * row_h, stat_size, label_col);
    sprintf(buf, "%d", gs->stats.damage_taken);
    vw = MeasureText(buf, stat_size);
    DrawText(buf, col2_x + col_w - vw, stat_y + 1 * row_h, stat_size, taken_col);

    DrawText("GEMS / ITEMS", col2_x, stat_y + 2 * row_h, stat_size, label_col);
    sprintf(buf, "%d / %d", gs->stats.gems_collected, gs->stats.items_collected);
    vw = MeasureText(buf, stat_size);
    DrawText(buf, col2_x + col_w - vw, stat_y + 2 * row_h, stat_size, gems_col);

    if (gs->boss_defeated) {
        const char *bm = "* FORMAT DEFEATED *";
        int bms = 14;
        int bmw = MeasureText(bm, bms);
        DrawText(bm, (sw - bmw) / 2, stat_y + row_h * 3 + 6, bms,
            (Color){255, 200, 50, 255});
    }

    // === Per-weapon DPS breakdown ===
    int bars_y = stat_y + row_h * 3 + 28;
    const char *bars_title = "WEAPONS";
    DrawText(bars_title, col1_x, bars_y, stat_size, label_col);
    const char *bars_h1 = "DPS";
    const char *bars_h2 = "%";
    int h1w = MeasureText(bars_h1, stat_size);
    int h2w = MeasureText(bars_h2, stat_size);
    int total_col_x = sw - 40 - 60 - h2w - 12;
    DrawText(bars_h1, total_col_x - h1w, bars_y, stat_size, label_col);
    DrawText(bars_h2, sw - 40 - h2w, bars_y, stat_size, label_col);

    // Sort indices by damage_dealt descending
    int order[WEAPON_ID_COUNT];
    for (int i = 0; i < WEAPON_ID_COUNT; i++) order[i] = i;
    for (int i = 0; i < WEAPON_ID_COUNT - 1; i++) {
        for (int j = i + 1; j < WEAPON_ID_COUNT; j++) {
            if (gs->stats.damage_dealt[order[j]] > gs->stats.damage_dealt[order[i]]) {
                int t = order[i]; order[i] = order[j]; order[j] = t;
            }
        }
    }

    int row_y = bars_y + 24;
    int row_step = 18;
    int name_x = col1_x;
    int bar_x = col1_x + 90;
    int bar_max_w = total_col_x - h1w - bar_x - 16;

    float time_div = (gs->game_time > 0.5f) ? gs->game_time : 1.0f;
    for (int rank = 0; rank < WEAPON_ID_COUNT; rank++) {
        int w = order[rank];
        int dmg = gs->stats.damage_dealt[w];
        if (dmg <= 0) continue;
        Color c = weapon_row_color((WeaponID)w);
        int y = row_y + rank * row_step;

        DrawText(WEAPON_NAMES[w], name_x, y, 13, c);

        float frac = total_dmg > 0 ? (float)dmg / total_dmg : 0;
        int bw = (int)(bar_max_w * frac);
        if (bw < 1) bw = 1;
        DrawRectangle(bar_x, y + 2, bar_max_w, 10, (Color){30, 30, 40, 255});
        DrawRectangle(bar_x, y + 2, bw, 10, (Color){c.r, c.g, c.b, 200});

        float weapon_dps = dmg / time_div;
        sprintf(buf, "%.1f", weapon_dps);
        int dw = MeasureText(buf, 13);
        DrawText(buf, total_col_x - dw, y, 13, value_col);

        sprintf(buf, "%d%%", (int)(frac * 100 + 0.5f));
        int pw = MeasureText(buf, 13);
        DrawText(buf, sw - 40 - pw, y, 13, label_col);
    }

    // === Prompt ===
    if (gs->scene_timer >= 0.5f) {
        float blink = sinf(gs->scene_timer * 3.0f);
        if (blink > -0.3f) {
            const char *prompt = "PRESS SPACE to return to TITLE";
            int ps = 14;
            int pw = MeasureText(prompt, ps);
            DrawText(prompt, (sw - pw) / 2, sh - 32, ps, (Color){180, 180, 200, 255});
        }
    }

}
