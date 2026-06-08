#include "../game.h"
#include "../weapon_util.h"
#include <math.h>

void whip_init(GameState *gs) {
    gs->whip.has = true;
    gs->whip.timer = WHIP_INTERVAL;
    gs->whip.anim = 0;
    gs->whip.interval = WHIP_INTERVAL;
    gs->whip.damage = WHIP_DAMAGE;
    gs->whip.arc = WHIP_ARC;
}

void whip_update(GameState *gs, float dt) {
    if (!gs->whip.has) return;

    if (gs->whip.anim > 0) gs->whip.anim -= dt;

    gs->whip.timer -= dt;
    if (gs->whip.timer > 0) return;
    gs->whip.timer = gs->whip.interval * gs->weapon_rate_mult;
    gs->whip.anim = WHIP_ANIM * gs->weapon_duration_mult;

    float center = gs->player.facing_angle;
    float range_eff = WHIP_RANGE * gs->weapon_area_mult;
    float r2 = range_eff * range_eff;

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!gs->enemies[i].active) continue;
        if (gs->enemies[i].phased) continue;
        float dx = gs->enemies[i].pos.x - gs->player.pos.x;
        float dy = gs->enemies[i].pos.y - gs->player.pos.y;
        float d2 = dx * dx + dy * dy;
        if (d2 > r2) continue;
        float ang = atan2f(dy, dx);
        float diff = ang - center;
        while (diff > 3.14159f) diff -= 2.0f * 3.14159f;
        while (diff < -3.14159f) diff += 2.0f * 3.14159f;
        if (fabsf(diff) < gs->whip.arc * 0.5f) {
            weapon_hit_enemy(gs, i, gs->whip.damage,
                             (Color){220, 220, 255, 255});
        }
    }
    if (gs->boss.active) {
        float dx = gs->boss.pos.x - gs->player.pos.x;
        float dy = gs->boss.pos.y - gs->player.pos.y;
        float d2 = dx * dx + dy * dy;
        if (d2 < r2) {
            float ang = atan2f(dy, dx);
            float diff = ang - center;
            while (diff > 3.14159f) diff -= 2.0f * 3.14159f;
            while (diff < -3.14159f) diff += 2.0f * 3.14159f;
            if (fabsf(diff) < gs->whip.arc * 0.5f) {
                weapon_hit_boss(gs, gs->whip.damage);
            }
        }
    }
    // Destroy enemy bullets within the arc
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (!gs->enemy_bullets[i].active) continue;
        float dx = gs->enemy_bullets[i].pos.x - gs->player.pos.x;
        float dy = gs->enemy_bullets[i].pos.y - gs->player.pos.y;
        float d2 = dx * dx + dy * dy;
        if (d2 > r2) continue;
        float ang = atan2f(dy, dx);
        float diff = ang - center;
        while (diff > 3.14159f) diff -= 2.0f * 3.14159f;
        while (diff < -3.14159f) diff += 2.0f * 3.14159f;
        if (fabsf(diff) < gs->whip.arc * 0.5f) {
            particles_spawn_burst(gs, gs->enemy_bullets[i].pos,
                                  (Color){255, 200, 200, 255}, 4);
            gs->enemy_bullets[i].active = false;
        }
    }
    audio_play(SFX_SHOOT);
}

void whip_draw(const GameState *gs, float scale, Vector2 offset) {
    if (!gs->whip.has || gs->whip.anim <= 0) return;

    float px = gs->player.pos.x * scale + offset.x;
    float py = gs->player.pos.y * scale + offset.y;
    float r = WHIP_RANGE * gs->weapon_area_mult * scale;
    float anim_max = WHIP_ANIM * gs->weapon_duration_mult;
    float alpha = (anim_max > 0) ? gs->whip.anim / anim_max : 0;
    float center = gs->player.facing_angle * (180.0f / 3.14159f);
    float arc_deg = gs->whip.arc * (180.0f / 3.14159f);

    Color outer = {220, 220, 255, (unsigned char)(180 * alpha)};
    Color inner = {255, 255, 255, (unsigned char)(220 * alpha)};

    DrawRing((Vector2){px, py}, r * 0.8f, r,
        center - arc_deg * 0.5f, center + arc_deg * 0.5f, 24, outer);
    DrawRing((Vector2){px, py}, r * 0.9f, r * 0.95f,
        center - arc_deg * 0.5f, center + arc_deg * 0.5f, 24, inner);
}
