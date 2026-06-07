#include "../game.h"
#include "../weapon_util.h"
#include <math.h>

static void fire_bullet(GameState *gs, Vector2 dir) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!gs->bullets[i].active) {
            gs->bullets[i].active = true;
            gs->bullets[i].pos = gs->player.pos;
            gs->bullets[i].vel.x = dir.x * BULLET_SPEED;
            gs->bullets[i].vel.y = dir.y * BULLET_SPEED;
            return;
        }
    }
}

void weapon_update(GameState *gs, float dt) {
    if (!gs->pulse.has) return;

    gs->pulse.fire_timer -= dt;
    if (gs->pulse.fire_timer <= 0) {
        gs->pulse.fire_timer = gs->pulse.fire_interval * gs->weapon_rate_mult;

        Vector2 target_pos;
        if (weapon_nearest_target(gs, gs->player.pos, &target_pos)) {
            float dx = target_pos.x - gs->player.pos.x;
            float dy = target_pos.y - gs->player.pos.y;
            float len = sqrtf(dx * dx + dy * dy);
            if (len > 0) {
                Vector2 dir = {dx / len, dy / len};
                int total = gs->pulse.bullet_count + gs->weapon_extra_projectiles;
                for (int i = 0; i < total; i++) {
                    fire_bullet(gs, dir);
                }
                audio_play(SFX_SHOOT);
            }
        }
    }
}

void bullet_update(GameState *gs, float dt) {
    Color popup_col = {255, 255, 200, 255};
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!gs->bullets[i].active) continue;

        Bullet *b = &gs->bullets[i];
        b->pos.x += b->vel.x * dt;
        b->pos.y += b->vel.y * dt;

        if (b->pos.x < -50 || b->pos.x > LOGICAL_W + 50 ||
            b->pos.y < -50 || b->pos.y > LOGICAL_H + 50) {
            b->active = false;
            continue;
        }

        if (gs->boss.active) {
            float dx = b->pos.x - gs->boss.pos.x;
            float dy = b->pos.y - gs->boss.pos.y;
            if (dx * dx + dy * dy <
                (BULLET_RADIUS + BOSS_RADIUS) * (BULLET_RADIUS + BOSS_RADIUS)) {
                weapon_hit_boss(gs, gs->pulse.damage);
                b->active = false;
                continue;
            }
        }

        for (int j = 0; j < MAX_ENEMIES; j++) {
            if (!gs->enemies[j].active) continue;
            if (gs->enemies[j].phased) continue;
            float dx = b->pos.x - gs->enemies[j].pos.x;
            float dy = b->pos.y - gs->enemies[j].pos.y;
            float reach = BULLET_RADIUS + gs->enemies[j].radius;
            if (dx * dx + dy * dy < reach * reach) {
                weapon_hit_enemy(gs, j, gs->pulse.damage, popup_col);
                b->active = false;
                break;
            }
        }
    }
}

void bullet_draw(const Bullet bullets[], float scale, Vector2 offset) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;
        float x = bullets[i].pos.x * scale + offset.x;
        float y = bullets[i].pos.y * scale + offset.y;
        float r = BULLET_RADIUS * scale;

        float vlen = sqrtf(bullets[i].vel.x * bullets[i].vel.x +
                           bullets[i].vel.y * bullets[i].vel.y);
        if (vlen < 0.001f) vlen = 1;
        float dx = bullets[i].vel.x / vlen;
        float dy = bullets[i].vel.y / vlen;
        float trail_len = r * 4.0f;
        Vector2 tail = {x - dx * trail_len, y - dy * trail_len};
        Vector2 head = {x + dx * r * 0.5f, y + dy * r * 0.5f};

        DrawLineEx(tail, head, r * 1.6f, (Color){255, 255, 100, 180});
        DrawLineEx(tail, head, r * 0.8f, (Color){255, 255, 220, 255});
        DrawCircleV((Vector2){x, y}, r * 0.5f, (Color){255, 255, 255, 255});
    }
}
