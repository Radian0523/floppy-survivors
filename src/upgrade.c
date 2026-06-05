#include "game.h"
#include <stdlib.h>
#include <stdio.h>

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
    "Orbiters +1",
    "Orbit radius +20",
    "Beam width +4",
    "Beam interval x0.8",
    "Nova range +30",
    "Nova damage +1"
};

static Color upgrade_colors[] = {
    {255, 200, 100, 255},
    {100, 200, 255, 255},
    {255, 100, 100, 255},
    {100, 255, 200, 255},
    {200, 100, 255, 255},
    {255, 100, 200, 255},
    {100, 150, 255, 255},
    {255, 80, 80, 255},
    {255, 100, 255, 255},
    {100, 150, 255, 255},
    {100, 150, 255, 255},
    {255, 80, 80, 255},
    {255, 80, 80, 255},
    {255, 100, 255, 255},
    {255, 100, 255, 255}
};

const char* upgrade_get_name(UpgradeType type) {
    return upgrade_names[type];
}

static bool is_upgrade_available(const GameState *gs, UpgradeType type) {
    switch (type) {
        case UPGRADE_ORBITERS:
            return !gs->has_orbiters;
        case UPGRADE_BEAM:
            return !gs->has_beam;
        case UPGRADE_NOVA:
            return !gs->has_nova;
        case UPGRADE_ORBITER_COUNT:
            return gs->has_orbiters && gs->orbiter_count < MAX_ORBITERS;
        case UPGRADE_ORBITER_RADIUS:
            return gs->has_orbiters;
        case UPGRADE_BEAM_WIDTH:
        case UPGRADE_BEAM_INTERVAL:
            return gs->has_beam;
        case UPGRADE_NOVA_RANGE:
        case UPGRADE_NOVA_DAMAGE:
            return gs->has_nova;
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

static void apply_upgrade(GameState *gs, UpgradeType type) {
    switch (type) {
        case UPGRADE_RAPID_FIRE:
            gs->fire_interval *= UPGRADE_RAPID_FIRE_MULT;
            break;
        case UPGRADE_MULTI_SHOT:
            gs->bullet_count += UPGRADE_MULTI_SHOT_ADD;
            break;
        case UPGRADE_POWER:
            gs->bullet_damage += UPGRADE_POWER_ADD;
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
        case UPGRADE_ORBITER_COUNT:
            if (gs->orbiter_count < MAX_ORBITERS) {
                gs->orbiters[gs->orbiter_count].active = true;
                gs->orbiters[gs->orbiter_count].angle = 0;
                gs->orbiter_count += UPGRADE_ORBITER_COUNT_ADD;
            }
            break;
        case UPGRADE_ORBITER_RADIUS:
            gs->orbiter_orbit_radius += UPGRADE_ORBITER_RADIUS_ADD;
            break;
        case UPGRADE_BEAM_WIDTH:
            gs->beam_width += UPGRADE_BEAM_WIDTH_ADD;
            break;
        case UPGRADE_BEAM_INTERVAL:
            gs->beam_interval *= UPGRADE_BEAM_INTERVAL_MULT;
            break;
        case UPGRADE_NOVA_RANGE:
            gs->nova_max_radius += UPGRADE_NOVA_RANGE_ADD;
            break;
        case UPGRADE_NOVA_DAMAGE:
            gs->nova_damage += UPGRADE_NOVA_DAMAGE_ADD;
            break;
        default:
            break;
    }
}

void upgrade_update(GameState *gs) {
    if (!gs->upgrading) return;

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

void upgrade_draw(const GameState *gs) {
    if (!gs->upgrading) return;

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0, 0, 0, 180});

    const char *title = "LEVEL UP!";
    int title_w = MeasureText(title, 40);
    DrawText(title, (GetScreenWidth() - title_w) / 2, GetScreenHeight() / 2 - 100, 40,
        (Color){255, 255, 100, 255});

    const char *hint = "Arrow/WASD: move  -  SPACE/Enter: confirm  -  1-3: quick select";
    int hint_w = MeasureText(hint, 16);
    DrawText(hint, (GetScreenWidth() - hint_w) / 2, GetScreenHeight() / 2 - 50, 16,
        (Color){180, 180, 180, 255});

    int box_w = 180;
    int box_h = 80;
    int gap = 20;
    int total_w = UPGRADE_CHOICES * box_w + (UPGRADE_CHOICES - 1) * gap;
    int start_x = (GetScreenWidth() - total_w) / 2;
    int start_y = GetScreenHeight() / 2 - box_h / 2 + 30;

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

        const char *name = upgrade_names[type];
        int name_w = MeasureText(name, 18);
        DrawText(name, x + (box_w - name_w) / 2, y + 20, 18, col);

        const char *desc = upgrade_descs[type];
        int desc_w = MeasureText(desc, 14);
        DrawText(desc, x + (box_w - desc_w) / 2, y + 50, 14, (Color){180, 180, 180, 255});
    }
}
