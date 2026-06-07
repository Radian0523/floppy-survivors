#include "../game.h"
#include "../weapon_util.h"
#include <math.h>

void trail_init(GameState *gs) {
    gs->trail.has = true;
    gs->trail.timer = 0;
    gs->trail.damage = TRAIL_DAMAGE;
    gs->trail.life = TRAIL_LIFE;
    for (int i = 0; i < MAX_TRAIL_MARKS; i++) gs->trail.slots[i].active = false;
}

void trail_update(GameState *gs, float dt) {
    if (!gs->trail.has) return;

    gs->trail.timer -= dt;
    float vx = gs->player.vel.x;
    float vy = gs->player.vel.y;
    bool moving = (vx * vx + vy * vy) > 1.0f;
    if (gs->trail.timer <= 0 && moving) {
        gs->trail.timer = TRAIL_INTERVAL;
        for (int i = 0; i < MAX_TRAIL_MARKS; i++) {
            if (!gs->trail.slots[i].active) {
                gs->trail.slots[i].active = true;
                gs->trail.slots[i].pos = gs->player.pos;
                gs->trail.slots[i].life = gs->trail.life * gs->weapon_duration_mult;
                break;
            }
        }
    }

    for (int i = 0; i < MAX_TRAIL_MARKS; i++) {
        if (!gs->trail.slots[i].active) continue;
        TrailMark *m = &gs->trail.slots[i];
        m->life -= dt;
        if (m->life <= 0) {
            m->active = false;
            continue;
        }
        float r_eff = TRAIL_RADIUS * gs->weapon_area_mult;
        for (int j = 0; j < MAX_ENEMIES; j++) {
            if (!gs->enemies[j].active) continue;
            if (gs->enemies[j].phased) continue;
            float dx = gs->enemies[j].pos.x - m->pos.x;
            float dy = gs->enemies[j].pos.y - m->pos.y;
            if (dx * dx + dy * dy <
                (r_eff + gs->enemies[j].radius) *
                (r_eff + gs->enemies[j].radius)) {
                weapon_hit_enemy(gs, j, gs->trail.damage,
                                 (Color){120, 220, 255, 255});
                m->life -= 0.3f;
            }
        }
    }
}

void trail_draw(const GameState *gs, float scale, Vector2 offset) {
    if (!gs->trail.has) return;
    float life_max = gs->trail.life * gs->weapon_duration_mult;
    for (int i = 0; i < MAX_TRAIL_MARKS; i++) {
        if (!gs->trail.slots[i].active) continue;
        float ratio = (life_max > 0) ? gs->trail.slots[i].life / life_max : 0;
        float x = gs->trail.slots[i].pos.x * scale + offset.x;
        float y = gs->trail.slots[i].pos.y * scale + offset.y;
        float r = TRAIL_RADIUS * gs->weapon_area_mult * scale * (0.6f + 0.4f * ratio);
        Color col = {120, 220, 255, (unsigned char)(180 * ratio)};
        Color inner = {200, 240, 255, (unsigned char)(220 * ratio)};
        DrawCircleV((Vector2){x, y}, r, col);
        DrawCircleV((Vector2){x, y}, r * 0.5f, inner);
    }
}
