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

// Find up to `n` nearest enemies/boss positions, write to out[].
// Returns number actually found.
static int find_n_nearest_targets(const GameState *gs, int n, Vector2 *out) {
    if (n <= 0) return 0;
    typedef struct { Vector2 pos; float d2; } Cand;
    Cand cands[MAX_ENEMIES + 1];
    int nc = 0;

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!gs->enemies[i].active) continue;
        if (gs->enemies[i].phased) continue;
        float dx = gs->enemies[i].pos.x - gs->player.pos.x;
        float dy = gs->enemies[i].pos.y - gs->player.pos.y;
        cands[nc].pos = gs->enemies[i].pos;
        cands[nc].d2 = dx * dx + dy * dy;
        nc++;
    }
    if (gs->boss.active) {
        float dx = gs->boss.pos.x - gs->player.pos.x;
        float dy = gs->boss.pos.y - gs->player.pos.y;
        cands[nc].pos = gs->boss.pos;
        cands[nc].d2 = dx * dx + dy * dy;
        nc++;
    }

    int take = (n < nc) ? n : nc;
    for (int i = 0; i < take; i++) {
        int min_j = i;
        for (int j = i + 1; j < nc; j++) {
            if (cands[j].d2 < cands[min_j].d2) min_j = j;
        }
        if (min_j != i) {
            Cand tmp = cands[i];
            cands[i] = cands[min_j];
            cands[min_j] = tmp;
        }
        out[i] = cands[i].pos;
    }
    return take;
}

void weapon_update(GameState *gs, float dt) {
    if (!gs->pulse.has) return;

    gs->pulse.fire_timer -= dt;
    if (gs->pulse.fire_timer <= 0) {
        gs->pulse.fire_timer = gs->pulse.fire_interval * gs->weapon_rate_mult;

        int total = gs->pulse.bullet_count + gs->weapon_extra_projectiles;
        Vector2 targets[MAX_ENEMIES + 1];
        int found = find_n_nearest_targets(gs, total, targets);

        for (int i = 0; i < found; i++) {
            float dx = targets[i].x - gs->player.pos.x;
            float dy = targets[i].y - gs->player.pos.y;
            float len = sqrtf(dx * dx + dy * dy);
            if (len > 0) {
                Vector2 dir = {dx / len, dy / len};
                fire_bullet(gs, dir);
            }
        }
        if (found > 0) audio_play(SFX_SHOOT);
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

        if (weapon_destroy_bullets_at(gs, b->pos, BULLET_RADIUS) > 0) {
            b->active = false;
            continue;
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
