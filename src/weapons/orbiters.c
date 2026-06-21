#include "../game.h"
#include "../weapon_util.h"
#include <math.h>

#define TWO_PI 6.28318530718f

void orbiters_init(GameState *gs) {
    gs->orbiters.has = true;
    gs->orbiters.count = ORBITER_COUNT_BASE;
    gs->orbiters.damage = ORBITER_DAMAGE;
    gs->orbiters.orbit_radius = ORBITER_ORBIT_RADIUS;
    gs->orbiters.base_angle = 0;
}

static inline float orb_angle(const OrbitersWeapon *o, int i) {
    return o->base_angle + (TWO_PI * (float)i) / (float)o->count;
}

void orbiters_update(GameState *gs, float dt) {
    if (!gs->orbiters.has) return;
    if (gs->orbiters.count <= 0) return;

    gs->orbiters.base_angle += ORBITER_SPEED * dt;
    if (gs->orbiters.base_angle > TWO_PI) gs->orbiters.base_angle -= TWO_PI;

    Color popup_col = {200, 220, 255, 255};
    for (int i = 0; i < gs->orbiters.count; i++) {
        float a = orb_angle(&gs->orbiters, i);
        Vector2 op = {
            gs->player.pos.x + cosf(a) * gs->orbiters.orbit_radius,
            gs->player.pos.y + sinf(a) * gs->orbiters.orbit_radius
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
    if (gs->orbiters.count <= 0) return;
    for (int i = 0; i < gs->orbiters.count; i++) {
        float a = orb_angle(&gs->orbiters, i);
        float ox = gs->player.pos.x + cosf(a) * gs->orbiters.orbit_radius;
        float oy = gs->player.pos.y + sinf(a) * gs->orbiters.orbit_radius;
        float x = ox * scale + offset.x;
        float y = oy * scale + offset.y;
        float r = ORBITER_RADIUS * scale;
        // Distinct shape vs trail (which is a thin ring decal):
        // ORBITERS = solid orb + faint outer halo. Reads as a tangible "object".
        DrawCircleV((Vector2){x, y}, r * 1.5f, (Color){100, 150, 255, 60});
        DrawCircleV((Vector2){x, y}, r, (Color){100, 150, 255, 255});
        DrawCircleV((Vector2){x, y}, r * 0.6f, (Color){200, 230, 255, 220});
        DrawCircleV((Vector2){x, y}, r * 0.3f, (Color){255, 255, 255, 255});
    }
}
