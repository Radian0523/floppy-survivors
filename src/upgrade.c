#include "game.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

static const char* upgrade_names[] = {
    "RAPID FIRE",
    "MULTI SHOT",
    "POWER",
    "SPEED",
    "MAGNET",
    "VITALITY",
    "ORBITERS",
    "BEAM",
    "NOVA",
    "MINES",
    "CHAIN",
    "BOOMERANG",
    "TRAIL",
    "WHIP",
    "ORB COUNT",
    "ORB ORBIT",
    "BEAM WIDTH",
    "BEAM RATE",
    "NOVA RANGE",
    "NOVA POWER"
};

static const char* upgrade_descs[] = {
    "Fire rate x0.82",
    "Bullets +1",
    "Damage +1",
    "Move speed x1.12",
    "Pickup range +34",
    "Max HP +1",
    "Orbiting shields",
    "Sweeping laser",
    "Pulse wave",
    "Deploy spark mines",
    "Lightning chain",
    "Returning blade",
    "Damaging trail",
    "Melee arc swing",
    "Orbiters +1",
    "Orbit radius +20",
    "Beam width +4",
    "Beam interval x0.8",
    "Nova range +30",
    "Nova damage +1"
};

static Color upgrade_colors[] = {
    {255, 255, 100, 255},   // RAPID FIRE - yellow (Pulse)
    {100, 200, 255, 255},   // MULTI SHOT
    {255, 255, 150, 255},   // POWER
    {120, 255, 200, 255},   // SPEED
    {120, 255, 120, 255},   // MAGNET - green
    {120, 255, 120, 255},   // VITALITY - green
    {100, 150, 255, 255},   // ORBITERS - blue
    {180, 80, 255, 255},    // BEAM - violet
    {100, 255, 255, 255},   // NOVA - cyan
    {255, 220, 100, 255},   // MINES - golden
    {200, 240, 255, 255},   // CHAIN - light cyan
    {150, 255, 180, 255},   // BOOMERANG - light green
    {120, 220, 255, 255},   // TRAIL - light blue
    {220, 220, 255, 255},   // WHIP - silver white
    {100, 150, 255, 255},   // ORB COUNT
    {100, 150, 255, 255},   // ORB ORBIT
    {180, 80, 255, 255},    // BEAM WIDTH
    {180, 80, 255, 255},    // BEAM RATE
    {100, 255, 255, 255},   // NOVA RANGE
    {100, 255, 255, 255}    // NOVA POWER
};

const char* upgrade_get_name(UpgradeType type) {
    return upgrade_names[type];
}

static bool is_upgrade_available(const GameState *gs, UpgradeType type) {
    switch (type) {
        case UPGRADE_ORBITERS:    return !gs->orbiters.has;
        case UPGRADE_BEAM:        return !gs->beam.has;
        case UPGRADE_NOVA:        return !gs->nova.has;
        case UPGRADE_MINES:       return !gs->mines.has;
        case UPGRADE_CHAIN:       return !gs->chain.has;
        case UPGRADE_BOOMERANG:   return !gs->boomerang.has;
        case UPGRADE_TRAIL:       return !gs->trail.has;
        case UPGRADE_WHIP:        return !gs->whip.has;
        case UPGRADE_ORBITER_COUNT:
            return gs->orbiters.has && gs->orbiters.count < MAX_ORBITERS;
        case UPGRADE_ORBITER_RADIUS:
            return gs->orbiters.has;
        case UPGRADE_BEAM_WIDTH:
        case UPGRADE_BEAM_INTERVAL:
            return gs->beam.has;
        case UPGRADE_NOVA_RANGE:
        case UPGRADE_NOVA_DAMAGE:
            return gs->nova.has;
        default:
            return true;
    }
}

void upgrade_start(GameState *gs) {
    gs->upgrading = true;
    gs->upgrade_hover = 0;

    bool used[UPGRADE_COUNT] = {false};

    for (int i = 0; i < UPGRADE_COUNT; i++) {
        if (!is_upgrade_available(gs, i)) {
            used[i] = true;
        }
    }

    for (int i = 0; i < UPGRADE_CHOICES; i++) {
        int choice;
        int attempts = 0;
        do {
            choice = rand() % UPGRADE_COUNT;
            attempts++;
            if (attempts > 100) {
                choice = rand() % 6;
                break;
            }
        } while (used[choice]);
        used[choice] = true;
        gs->upgrade_choices[i] = choice;
    }
}

static void apply_upgrade(GameState *gs, UpgradeType type);

void upgrade_auto_pick(GameState *gs) {
    if (!gs->upgrading) return;
    int idx = rand() % UPGRADE_CHOICES;
    apply_upgrade(gs, gs->upgrade_choices[idx]);
    gs->upgrading = false;
}

static void apply_upgrade(GameState *gs, UpgradeType type) {
    switch (type) {
        case UPGRADE_RAPID_FIRE:
            gs->weapon_rate_mult *= UPGRADE_RAPID_FIRE_MULT;
            break;
        case UPGRADE_MULTI_SHOT:
            gs->weapon_extra_projectiles += UPGRADE_MULTI_SHOT_ADD;
            break;
        case UPGRADE_POWER:
            gs->weapon_damage_bonus += UPGRADE_POWER_ADD;
            break;
        case UPGRADE_SPEED:
            gs->player.speed *= UPGRADE_SPEED_MULT;
            break;
        case UPGRADE_MAGNET:
            gs->player.pickup_range += UPGRADE_MAGNET_ADD;
            break;
        case UPGRADE_VITALITY:
            gs->player.max_hp += UPGRADE_VITALITY_ADD;
            gs->player.hp += UPGRADE_VITALITY_ADD;
            break;
        case UPGRADE_ORBITERS:
            orbiters_init(gs);
            break;
        case UPGRADE_BEAM:
            beam_init(gs);
            break;
        case UPGRADE_NOVA:
            nova_init(gs);
            break;
        case UPGRADE_MINES:
            mines_init(gs);
            break;
        case UPGRADE_CHAIN:
            chain_init(gs);
            break;
        case UPGRADE_BOOMERANG:
            boomerang_init(gs);
            break;
        case UPGRADE_TRAIL:
            trail_init(gs);
            break;
        case UPGRADE_WHIP:
            whip_init(gs);
            break;
        case UPGRADE_ORBITER_COUNT:
            if (gs->orbiters.count < MAX_ORBITERS) {
                gs->orbiters.slots[gs->orbiters.count].active = true;
                gs->orbiters.slots[gs->orbiters.count].angle = 0;
                gs->orbiters.count += UPGRADE_ORBITER_COUNT_ADD;
            }
            break;
        case UPGRADE_ORBITER_RADIUS:
            gs->orbiters.orbit_radius += UPGRADE_ORBITER_RADIUS_ADD;
            break;
        case UPGRADE_BEAM_WIDTH:
            gs->beam.width += UPGRADE_BEAM_WIDTH_ADD;
            break;
        case UPGRADE_BEAM_INTERVAL:
            gs->beam.interval *= UPGRADE_BEAM_INTERVAL_MULT;
            break;
        case UPGRADE_NOVA_RANGE:
            gs->nova.max_radius += UPGRADE_NOVA_RANGE_ADD;
            break;
        case UPGRADE_NOVA_DAMAGE:
            gs->nova.damage += UPGRADE_NOVA_DAMAGE_ADD;
            break;
        default:
            break;
    }
}

void upgrade_update(GameState *gs) {
    if (!gs->upgrading) return;

    if (gs->bot_mode) {
        upgrade_auto_pick(gs);
        return;
    }

    int box_w = 180;
    int box_h = 80;
    int gap = 20;
    int total_w = UPGRADE_CHOICES * box_w + (UPGRADE_CHOICES - 1) * gap;
    int start_x = (GetScreenWidth() - total_w) / 2;
    int start_y = GetScreenHeight() / 2 - box_h / 2 + 30;

    Vector2 mouse = GetMousePosition();
    static Vector2 prev_mouse = {0, 0};
    bool mouse_moved = (mouse.x != prev_mouse.x || mouse.y != prev_mouse.y);
    prev_mouse = mouse;

    if (mouse_moved) {
        int hovered = -1;
        for (int i = 0; i < UPGRADE_CHOICES; i++) {
            int x = start_x + i * (box_w + gap);
            int y = start_y;
            if (mouse.x >= x && mouse.x < x + box_w &&
                mouse.y >= y && mouse.y < y + box_h) {
                hovered = i;
            }
        }
        if (hovered >= 0) gs->upgrade_hover = hovered;
    }

    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        if (gs->upgrade_hover <= 0) gs->upgrade_hover = UPGRADE_CHOICES - 1;
        else gs->upgrade_hover--;
    }
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        if (gs->upgrade_hover < 0) gs->upgrade_hover = 0;
        else gs->upgrade_hover = (gs->upgrade_hover + 1) % UPGRADE_CHOICES;
    }

    int selected = -1;

    if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) selected = 0;
    if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) selected = 1;
    if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) selected = 2;

    if ((IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) && gs->upgrade_hover >= 0) {
        selected = gs->upgrade_hover;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && gs->upgrade_hover >= 0) {
        int x = start_x + gs->upgrade_hover * (box_w + gap);
        if (mouse.x >= x && mouse.x < x + box_w &&
            mouse.y >= start_y && mouse.y < start_y + box_h) {
            selected = gs->upgrade_hover;
        }
    }

    if (selected >= 0 && selected < UPGRADE_CHOICES) {
        apply_upgrade(gs, gs->upgrade_choices[selected]);
        gs->upgrading = false;
    }
}

typedef enum {
    CAT_NEW,        // new weapon unlock
    CAT_WEAPON_UP,  // improves an existing weapon
    CAT_PASSIVE,    // player passive
} UpgradeCategory;

static UpgradeCategory upgrade_category(UpgradeType type) {
    switch (type) {
        case UPGRADE_ORBITERS:
        case UPGRADE_BEAM:
        case UPGRADE_NOVA:
        case UPGRADE_MINES:
        case UPGRADE_CHAIN:
        case UPGRADE_BOOMERANG:
        case UPGRADE_TRAIL:
        case UPGRADE_WHIP:
            return CAT_NEW;
        case UPGRADE_SPEED:
        case UPGRADE_MAGNET:
        case UPGRADE_VITALITY:
            return CAT_PASSIVE;
        default:
            return CAT_WEAPON_UP;
    }
}

static const char *category_label(UpgradeCategory c) {
    switch (c) {
        case CAT_NEW:       return "NEW";
        case CAT_WEAPON_UP: return "WEAPON+";
        case CAT_PASSIVE:   return "PASSIVE";
        default:            return "";
    }
}

static Color category_color(UpgradeCategory c) {
    switch (c) {
        case CAT_NEW:       return (Color){255, 220, 80, 255};   // gold (new!)
        case CAT_WEAPON_UP: return (Color){255, 140, 80, 255};   // orange
        case CAT_PASSIVE:   return (Color){120, 200, 255, 255};  // blue
        default:            return (Color){180, 180, 180, 255};
    }
}

void upgrade_draw_preview(UpgradeType type, float cx, float cy, float t) {
    Color col = upgrade_colors[type];
    Color white = {255, 255, 255, 255};

    switch (type) {
        case UPGRADE_RAPID_FIRE: {
            // 3 fast bullets
            for (int k = 0; k < 3; k++) {
                float off = fmodf(t * 90.0f + k * 18.0f, 60.0f) - 30.0f;
                Vector2 head = {cx + off + 6, cy};
                Vector2 tail = {cx + off - 6, cy};
                DrawLineEx(tail, head, 3.0f, col);
            }
            break;
        }
        case UPGRADE_MULTI_SHOT: {
            // 3 streaks fanning out
            for (int k = -1; k <= 1; k++) {
                float a = k * 0.35f;
                Vector2 s = {cx - cosf(a) * 8, cy - sinf(a) * 8};
                Vector2 e = {cx + cosf(a) * 18, cy + sinf(a) * 18};
                DrawLineEx(s, e, 3.0f, col);
            }
            break;
        }
        case UPGRADE_POWER: {
            // Big bullet + impact star
            DrawCircleV((Vector2){cx, cy}, 7, col);
            DrawCircleV((Vector2){cx, cy}, 3, white);
            for (int k = 0; k < 6; k++) {
                float a = k * 1.047f;
                Vector2 s = {cx + cosf(a) * 10, cy + sinf(a) * 10};
                Vector2 e = {cx + cosf(a) * 16, cy + sinf(a) * 16};
                DrawLineEx(s, e, 1.5f, col);
            }
            break;
        }
        case UPGRADE_SPEED: {
            // Forward arrow with motion lines
            Vector2 tip = {cx + 14, cy};
            Vector2 ul = {cx + 4, cy - 8};
            Vector2 ll = {cx + 4, cy + 8};
            DrawTriangle(tip, ul, ll, col);
            DrawTriangle(tip, ll, ul, col);
            DrawLineEx((Vector2){cx - 16, cy - 5}, (Vector2){cx - 6, cy - 5}, 2, col);
            DrawLineEx((Vector2){cx - 16, cy + 5}, (Vector2){cx - 6, cy + 5}, 2, col);
            DrawLineEx((Vector2){cx - 14, cy}, (Vector2){cx - 6, cy}, 2, col);
            break;
        }
        case UPGRADE_MAGNET: {
            // U-shaped magnet pulling little gems
            DrawLineEx((Vector2){cx - 8, cy - 8}, (Vector2){cx - 8, cy + 6}, 4, col);
            DrawLineEx((Vector2){cx + 0, cy - 8}, (Vector2){cx + 0, cy + 6}, 4, col);
            DrawLineEx((Vector2){cx - 8, cy + 6}, (Vector2){cx + 0, cy + 6}, 4, col);
            // attracted gems
            float pull = fmodf(t * 30.0f, 20.0f);
            DrawCircleV((Vector2){cx + 12 + pull, cy - 6}, 2, col);
            DrawCircleV((Vector2){cx + 14 + pull * 0.6f, cy + 4}, 2, col);
            break;
        }
        case UPGRADE_VITALITY: {
            // Plus / cross
            DrawRectangle((int)(cx - 3), (int)(cy - 10), 6, 20, col);
            DrawRectangle((int)(cx - 10), (int)(cy - 3), 20, 6, col);
            DrawRectangle((int)(cx - 2), (int)(cy - 9), 4, 18, white);
            DrawRectangle((int)(cx - 9), (int)(cy - 2), 18, 4, white);
            break;
        }
        case UPGRADE_ORBITERS:
        case UPGRADE_ORBITER_COUNT: {
            DrawCircleV((Vector2){cx, cy}, 4, (Color){100, 255, 255, 255});
            int n = (type == UPGRADE_ORBITERS) ? 3 : 4;
            for (int k = 0; k < n; k++) {
                float a = t * 2.0f + (2.0f * 3.14159f * k / n);
                Vector2 p = {cx + cosf(a) * 16, cy + sinf(a) * 16};
                DrawCircleV(p, 4, col);
            }
            break;
        }
        case UPGRADE_ORBITER_RADIUS: {
            DrawCircleV((Vector2){cx, cy}, 4, (Color){100, 255, 255, 255});
            for (int k = 0; k < 3; k++) {
                float a = t * 2.0f + (2.0f * 3.14159f * k / 3);
                Vector2 p = {cx + cosf(a) * 22, cy + sinf(a) * 22};
                DrawCircleV(p, 4, col);
            }
            DrawRing((Vector2){cx, cy}, 12, 13, 0, 360, 24, (Color){col.r, col.g, col.b, 100});
            break;
        }
        case UPGRADE_BEAM:
        case UPGRADE_BEAM_WIDTH:
        case UPGRADE_BEAM_INTERVAL: {
            DrawCircleV((Vector2){cx, cy}, 4, (Color){100, 255, 255, 255});
            float a = sinf(t * 2.0f) * 0.7f;
            float w = (type == UPGRADE_BEAM_WIDTH) ? 5.0f : 3.0f;
            Vector2 e = {cx + cosf(a) * 24, cy + sinf(a) * 24};
            DrawLineEx((Vector2){cx, cy}, e, w, col);
            break;
        }
        case UPGRADE_NOVA:
        case UPGRADE_NOVA_RANGE:
        case UPGRADE_NOVA_DAMAGE: {
            DrawCircleV((Vector2){cx, cy}, 4, (Color){100, 255, 255, 255});
            float r_max = (type == UPGRADE_NOVA_RANGE) ? 28.0f : 22.0f;
            float r = fmodf(t * 35.0f, r_max);
            float alpha = 1.0f - r / r_max;
            Color rc = {col.r, col.g, col.b, (unsigned char)(255 * alpha)};
            DrawRing((Vector2){cx, cy}, r - 2, r + 2, 0, 360, 24, rc);
            if (type == UPGRADE_NOVA_DAMAGE) {
                DrawRing((Vector2){cx, cy}, r - 4, r + 4, 0, 360, 24,
                         (Color){col.r, col.g, col.b, (unsigned char)(120 * alpha)});
            }
            break;
        }
        case UPGRADE_MINES: {
            // 3 X marks at fixed positions, pulsing
            float pulse = 0.7f + 0.3f * sinf(t * 4.0f);
            Color cc = {col.r, col.g, col.b, (unsigned char)(255 * pulse)};
            for (int k = 0; k < 3; k++) {
                float mx = cx - 14 + k * 14;
                float my = cy + sinf(t * 1.5f + k) * 2.0f;
                DrawLineEx((Vector2){mx - 4, my - 4}, (Vector2){mx + 4, my + 4}, 2, cc);
                DrawLineEx((Vector2){mx + 4, my - 4}, (Vector2){mx - 4, my + 4}, 2, cc);
                DrawCircleV((Vector2){mx, my}, 2, cc);
            }
            break;
        }
        case UPGRADE_CHAIN: {
            // Zigzag lightning
            float off = sinf(t * 4.0f) * 3.0f;
            Vector2 pts[5] = {
                {cx - 20, cy + off},
                {cx - 10, cy - 6 + off},
                {cx, cy + 6 + off},
                {cx + 10, cy - 4 + off},
                {cx + 20, cy + off}
            };
            for (int k = 0; k < 4; k++) {
                DrawLineEx(pts[k], pts[k + 1], 2.5f, col);
            }
            for (int k = 0; k < 5; k++) DrawCircleV(pts[k], 2.5f, col);
            break;
        }
        case UPGRADE_BOOMERANG: {
            // Spinning X
            DrawCircleV((Vector2){cx, cy}, 4, (Color){100, 255, 255, 255});
            float spin = t * 8.0f;
            for (int k = 0; k < 2; k++) {
                float a = spin + k * 3.14159f;
                Vector2 e1 = {cx + cosf(a) * 12, cy + sinf(a) * 12};
                Vector2 e2 = {cx - cosf(a) * 12, cy - sinf(a) * 12};
                DrawLineEx(e1, e2, 3.0f, col);
            }
            break;
        }
        case UPGRADE_TRAIL: {
            // Dotted trail behind a circle
            DrawCircleV((Vector2){cx + 14, cy}, 4, (Color){100, 255, 255, 255});
            for (int k = 0; k < 5; k++) {
                float fade = 1.0f - k * 0.18f;
                float r = 4.0f - k * 0.5f;
                Color cc = {col.r, col.g, col.b, (unsigned char)(255 * fade * 0.7f)};
                DrawCircleV((Vector2){cx + 6 - k * 5, cy}, r, cc);
            }
            break;
        }
        case UPGRADE_WHIP: {
            // Arc swing
            DrawCircleV((Vector2){cx, cy}, 4, (Color){100, 255, 255, 255});
            float arc_start = sinf(t * 3.0f) * 50.0f;
            DrawRing((Vector2){cx, cy}, 15, 18, arc_start - 30, arc_start + 30, 24, col);
            break;
        }
        default:
            DrawCircleV((Vector2){cx, cy}, 6, col);
            break;
    }
}

void upgrade_draw(const GameState *gs) {
    if (!gs->upgrading) return;

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0, 0, 0, 180});

    const char *title = "LEVEL UP!";
    int title_w = MeasureText(title, 40);
    DrawText(title, (GetScreenWidth() - title_w) / 2, GetScreenHeight() / 2 - 140, 40,
        (Color){255, 255, 100, 255});

    const char *hint = "Arrow/WASD: move  -  SPACE/Enter: confirm  -  1-3: quick select";
    int hint_w = MeasureText(hint, 16);
    DrawText(hint, (GetScreenWidth() - hint_w) / 2, GetScreenHeight() / 2 - 95, 16,
        (Color){180, 180, 180, 255});

    int box_w = 180;
    int box_h = 140;
    int gap = 20;
    int total_w = UPGRADE_CHOICES * box_w + (UPGRADE_CHOICES - 1) * gap;
    int start_x = (GetScreenWidth() - total_w) / 2;
    int start_y = GetScreenHeight() / 2 - box_h / 2;

    float t = (float)GetTime();

    for (int i = 0; i < UPGRADE_CHOICES; i++) {
        int x = start_x + i * (box_w + gap);
        int y = start_y;

        UpgradeType type = gs->upgrade_choices[i];
        Color col = upgrade_colors[type];
        bool hover = (gs->upgrade_hover == i);

        if (hover) {
            DrawRectangle(x - 3, y - 3, box_w + 6, box_h + 6, col);
        }

        DrawRectangle(x, y, box_w, box_h, (Color){20, 20, 30, 255});
        DrawRectangleLines(x, y, box_w, box_h, col);

        char num[4];
        sprintf(num, "%d", i + 1);
        DrawText(num, x + 8, y + 8, 16, (Color){150, 150, 150, 255});

        // Category tag (top-right)
        UpgradeCategory cat = upgrade_category(type);
        const char *tag = category_label(cat);
        Color tag_col = category_color(cat);
        int tag_size = 10;
        int tag_text_w = MeasureText(tag, tag_size);
        int tag_pad_x = 6;
        int tag_pad_y = 3;
        int tag_w = tag_text_w + tag_pad_x * 2;
        int tag_h = tag_size + tag_pad_y * 2;
        int tag_x = x + box_w - tag_w - 6;
        int tag_y = y + 6;
        DrawRectangle(tag_x, tag_y, tag_w, tag_h, (Color){0, 0, 0, 200});
        DrawRectangleLines(tag_x, tag_y, tag_w, tag_h, tag_col);
        DrawText(tag, tag_x + tag_pad_x, tag_y + tag_pad_y, tag_size, tag_col);

        const char *name = upgrade_names[type];
        int name_w = MeasureText(name, 18);
        DrawText(name, x + (box_w - name_w) / 2, y + 26, 18, col);

        // Preview in the middle
        upgrade_draw_preview(type, x + box_w / 2.0f, y + 80.0f, t);

        const char *desc = upgrade_descs[type];
        int desc_w = MeasureText(desc, 13);
        DrawText(desc, x + (box_w - desc_w) / 2, y + box_h - 24, 13,
                 (Color){180, 180, 180, 255});
    }
}
