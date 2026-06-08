#include "../game.h"
#include "../weapon_util.h"
#include <math.h>

void beam_init(GameState *gs) {
    gs->beam.has = true;
    gs->beam.angle = 0;
    gs->beam.timer = BEAM_INTERVAL;
    gs->beam.firing = false;
    gs->beam.fire_timer = 0;
    gs->beam.interval = BEAM_INTERVAL;
    gs->beam.damage = BEAM_DAMAGE;
    gs->beam.length = BEAM_LENGTH;
    gs->beam.width = BEAM_WIDTH;
    gs->beam.sweep_angle = BEAM_SWEEP_ANGLE;
}

void beam_update(GameState *gs, float dt) {
    if (!gs->beam.has) return;

    Color popup_col = {255, 150, 150, 255};

    if (gs->beam.firing) {
        gs->beam.fire_timer -= dt;
        if (gs->beam.fire_timer <= 0) {
            gs->beam.firing = false;
            gs->beam.timer = gs->beam.interval * gs->weapon_rate_mult;
        }

        float dur = BEAM_DURATION * gs->weapon_duration_mult;
        float progress = 1.0f - (gs->beam.fire_timer / dur);
        gs->beam.angle = gs->beam.center_angle + gs->beam.sweep_angle -
                         (progress * 2.0f * gs->beam.sweep_angle);

        float cos_a = cosf(gs->beam.angle);
        float sin_a = sinf(gs->beam.angle);

        float eff_width = gs->beam.width * gs->weapon_area_mult;
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!gs->enemies[i].active) continue;
            if (gs->enemies[i].phased) continue;

            float dx = gs->enemies[i].pos.x - gs->player.pos.x;
            float dy = gs->enemies[i].pos.y - gs->player.pos.y;
            float proj = dx * cos_a + dy * sin_a;
            if (proj < 0 || proj > gs->beam.length) continue;
            float perp = fabsf(-dx * sin_a + dy * cos_a);
            if (perp < eff_width / 2 + gs->enemies[i].radius) {
                weapon_hit_enemy(gs, i, gs->beam.damage, popup_col);
            }
        }

        if (gs->boss.active) {
            float dx = gs->boss.pos.x - gs->player.pos.x;
            float dy = gs->boss.pos.y - gs->player.pos.y;
            float proj = dx * cos_a + dy * sin_a;
            if (proj >= 0 && proj <= gs->beam.length) {
                float perp = fabsf(-dx * sin_a + dy * cos_a);
                if (perp < eff_width / 2 + BOSS_RADIUS) {
                    weapon_hit_boss(gs, gs->beam.damage);
                }
            }
        }

        // Destroy enemy bullets along the beam line
        for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
            if (!gs->enemy_bullets[i].active) continue;
            float dx = gs->enemy_bullets[i].pos.x - gs->player.pos.x;
            float dy = gs->enemy_bullets[i].pos.y - gs->player.pos.y;
            float proj = dx * cos_a + dy * sin_a;
            if (proj < 0 || proj > gs->beam.length) continue;
            float perp = fabsf(-dx * sin_a + dy * cos_a);
            if (perp < eff_width / 2 + ENEMY_BULLET_RADIUS) {
                particles_spawn_burst(gs, gs->enemy_bullets[i].pos,
                                      (Color){255, 200, 200, 255}, 4);
                gs->enemy_bullets[i].active = false;
            }
        }
    } else {
        gs->beam.timer -= dt;
        if (gs->beam.timer <= 0) {
            gs->beam.firing = true;
            gs->beam.fire_timer = BEAM_DURATION * gs->weapon_duration_mult;
            gs->beam.center_angle = gs->player.facing_angle;
            gs->beam.angle = gs->beam.center_angle + gs->beam.sweep_angle;
            audio_play(SFX_BEAM);
        }
    }
}

void beam_draw(const GameState *gs, float scale, Vector2 offset) {
    if (!gs->beam.has || !gs->beam.firing) return;
    float px = gs->player.pos.x * scale + offset.x;
    float py = gs->player.pos.y * scale + offset.y;
    float ex = (gs->player.pos.x + cosf(gs->beam.angle) * gs->beam.length) * scale + offset.x;
    float ey = (gs->player.pos.y + sinf(gs->beam.angle) * gs->beam.length) * scale + offset.y;
    float w = gs->beam.width * gs->weapon_area_mult * scale;
    DrawLineEx((Vector2){px, py}, (Vector2){ex, ey}, w, (Color){180, 80, 255, 200});
    DrawLineEx((Vector2){px, py}, (Vector2){ex, ey}, w * 0.5f, (Color){220, 180, 255, 255});
}
