#include "../game.h"
#include "../weapon_util.h"
#include <math.h>

void orbiters_init(GameState *gs) {
    gs->orbiters.has = true;
    gs->orbiters.count = ORBITER_COUNT_BASE;
    gs->orbiters.damage = ORBITER_DAMAGE;
    gs->orbiters.orbit_radius = ORBITER_ORBIT_RADIUS;
    for (int i = 0; i < MAX_ORBITERS; i++) {
        gs->orbiters.slots[i].active = (i < gs->orbiters.count);
        gs->orbiters.slots[i].angle = (2.0f * 3.14159f * i) / gs->orbiters.count;
    }
}

void orbiters_update(GameState *gs, float dt) {
    if (!gs->orbiters.has) return;

    Color popup_col = {200, 220, 255, 255};
    for (int i = 0; i < gs->orbiters.count; i++) {
        if (!gs->orbiters.slots[i].active) continue;

        gs->orbiters.slots[i].angle += ORBITER_SPEED * dt;
        if (gs->orbiters.slots[i].angle > 2.0f * 3.14159f)
            gs->orbiters.slots[i].angle -= 2.0f * 3.14159f;

        Vector2 op = {
            gs->player.pos.x + cosf(gs->orbiters.slots[i].angle) * gs->orbiters.orbit_radius,
            gs->player.pos.y + sinf(gs->orbiters.slots[i].angle) * gs->orbiters.orbit_radius
        };

        for (int j = 0; j < MAX_ENEMIES; j++) {
            if (!gs->enemies[j].active) continue;
            if (gs->enemies[j].phased) continue;
            float dx = op.x - gs->enemies[j].pos.x;
            float dy = op.y - gs->enemies[j].pos.y;
            float reach = ORBITER_RADIUS + gs->enemies[j].radius;
            if (dx * dx + dy * dy < reach * reach) {
                weapon_hit_enemy(gs, j, gs->orbiters.damage, popup_col, WEAPON_ID_ORBITERS);
            }
        }
        weapon_try_hit_boss_radius(gs, op, ORBITER_RADIUS, gs->orbiters.damage, WEAPON_ID_ORBITERS);
        weapon_destroy_bullets_at(gs, op, ORBITER_RADIUS);
    }
}

void orbiters_draw(const GameState *gs, float scale, Vector2 offset) {
    if (!gs->orbiters.has) return;
    for (int i = 0; i < gs->orbiters.count; i++) {
        if (!gs->orbiters.slots[i].active) continue;
        float ox = gs->player.pos.x + cosf(gs->orbiters.slots[i].angle) * gs->orbiters.orbit_radius;
        float oy = gs->player.pos.y + sinf(gs->orbiters.slots[i].angle) * gs->orbiters.orbit_radius;
        float x = ox * scale + offset.x;
        float y = oy * scale + offset.y;
        float r = ORBITER_RADIUS * scale;
        DrawCircleV((Vector2){x, y}, r, (Color){100, 150, 255, 255});
        DrawCircleV((Vector2){x, y}, r * 0.6f, (Color){150, 200, 255, 200});
    }
}
