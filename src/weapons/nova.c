#include "../game.h"
#include "../weapon_util.h"
#include <math.h>

void nova_init(GameState *gs) {
    gs->nova.has = true;
    gs->nova.timer = NOVA_INTERVAL;
    gs->nova.current_radius = 0;
    gs->nova.expanding = false;
    gs->nova.interval = NOVA_INTERVAL;
    gs->nova.damage = NOVA_DAMAGE;
    gs->nova.max_radius = NOVA_RADIUS_BASE;
}

void nova_update(GameState *gs, float dt) {
    if (!gs->nova.has) return;

    Color popup_col = {255, 200, 255, 255};

    float eff_max = gs->nova.max_radius * gs->weapon_area_mult;

    if (gs->nova.expanding) {
        gs->nova.current_radius += NOVA_EXPAND_SPEED * dt;

        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!gs->enemies[i].active) continue;
            if (gs->enemies[i].phased) continue;
            float dx = gs->enemies[i].pos.x - gs->player.pos.x;
            float dy = gs->enemies[i].pos.y - gs->player.pos.y;
            float dist = sqrtf(dx * dx + dy * dy);
            float ring_inner = gs->nova.current_radius - 15.0f;
            float ring_outer = gs->nova.current_radius + 15.0f;
            if (dist > ring_inner && dist < ring_outer) {
                weapon_hit_enemy(gs, i, gs->nova.damage, popup_col);
            }
        }

        if (gs->boss.active) {
            float dx = gs->boss.pos.x - gs->player.pos.x;
            float dy = gs->boss.pos.y - gs->player.pos.y;
            float dist = sqrtf(dx * dx + dy * dy);
            float ring_inner = gs->nova.current_radius - 15.0f;
            float ring_outer = gs->nova.current_radius + 15.0f;
            if (dist > ring_inner - BOSS_RADIUS && dist < ring_outer + BOSS_RADIUS) {
                weapon_hit_boss(gs, gs->nova.damage);
            }
        }

        if (gs->nova.current_radius >= eff_max) {
            gs->nova.expanding = false;
            gs->nova.timer = gs->nova.interval * gs->weapon_rate_mult;
        }
    } else {
        gs->nova.timer -= dt;
        if (gs->nova.timer <= 0) {
            gs->nova.expanding = true;
            gs->nova.current_radius = 0;
            audio_play(SFX_NOVA);
        }
    }
}

void nova_draw(const GameState *gs, float scale, Vector2 offset) {
    if (!gs->nova.has || !gs->nova.expanding) return;
    float px = gs->player.pos.x * scale + offset.x;
    float py = gs->player.pos.y * scale + offset.y;
    float r = gs->nova.current_radius * scale;
    float eff_max = gs->nova.max_radius * gs->weapon_area_mult;
    float alpha = 1.0f - (gs->nova.current_radius / eff_max);
    Color col = {100, 255, 255, (unsigned char)(200 * alpha)};
    Color col_inner = {180, 255, 255, (unsigned char)(255 * alpha)};
    DrawRing((Vector2){px, py}, r - 5 * scale, r + 5 * scale, 0, 360, 36, col);
    DrawRing((Vector2){px, py}, r - 2 * scale, r + 2 * scale, 0, 360, 36, col_inner);
}
