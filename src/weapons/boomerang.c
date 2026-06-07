#include "../game.h"
#include "../weapon_util.h"
#include <math.h>

void boomerang_init(GameState *gs) {
    gs->boomerang.has = true;
    gs->boomerang.timer = BOOMERANG_INTERVAL;
    gs->boomerang.interval = BOOMERANG_INTERVAL;
    gs->boomerang.damage = BOOMERANG_DAMAGE;
    gs->boomerang.radius = BOOMERANG_RADIUS;
    for (int i = 0; i < MAX_BOOMERANGS; i++) gs->boomerang.slots[i].active = false;
}

static void throw_boomerang(GameState *gs, Vector2 dir) {
    for (int i = 0; i < MAX_BOOMERANGS; i++) {
        if (!gs->boomerang.slots[i].active) {
            gs->boomerang.slots[i].active = true;
            gs->boomerang.slots[i].pos = gs->player.pos;
            gs->boomerang.slots[i].dir = dir;
            gs->boomerang.slots[i].traveled = 0;
            gs->boomerang.slots[i].returning = false;
            return;
        }
    }
}

void boomerang_update(GameState *gs, float dt) {
    if (!gs->boomerang.has) return;

    gs->boomerang.timer -= dt;
    if (gs->boomerang.timer <= 0) {
        gs->boomerang.timer = gs->boomerang.interval * gs->weapon_rate_mult;
        int total = 1 + gs->weapon_extra_projectiles;
        float spread = 0.18f;
        float base = gs->player.facing_angle - spread * (total - 1) * 0.5f;
        for (int k = 0; k < total; k++) {
            float a = base + k * spread;
            Vector2 dir = {cosf(a), sinf(a)};
            throw_boomerang(gs, dir);
        }
    }

    for (int i = 0; i < MAX_BOOMERANGS; i++) {
        if (!gs->boomerang.slots[i].active) continue;
        BoomerangProj *b = &gs->boomerang.slots[i];

        Vector2 move_dir;
        if (b->returning) {
            float dx = gs->player.pos.x - b->pos.x;
            float dy = gs->player.pos.y - b->pos.y;
            float len = sqrtf(dx * dx + dy * dy);
            if (len < gs->boomerang.radius) {
                b->active = false;
                continue;
            }
            move_dir.x = dx / len;
            move_dir.y = dy / len;
        } else {
            move_dir = b->dir;
            b->traveled += BOOMERANG_SPEED * dt;
            if (b->traveled >= BOOMERANG_RANGE) b->returning = true;
        }

        b->pos.x += move_dir.x * BOOMERANG_SPEED * dt;
        b->pos.y += move_dir.y * BOOMERANG_SPEED * dt;

        for (int j = 0; j < MAX_ENEMIES; j++) {
            if (!gs->enemies[j].active) continue;
            if (gs->enemies[j].phased) continue;
            float dx = b->pos.x - gs->enemies[j].pos.x;
            float dy = b->pos.y - gs->enemies[j].pos.y;
            if (dx * dx + dy * dy <
                (gs->boomerang.radius + gs->enemies[j].radius) *
                (gs->boomerang.radius + gs->enemies[j].radius)) {
                weapon_hit_enemy(gs, j, gs->boomerang.damage,
                                 (Color){150, 255, 180, 255});
            }
        }
        if (gs->boss.active) {
            float dx = b->pos.x - gs->boss.pos.x;
            float dy = b->pos.y - gs->boss.pos.y;
            if (dx * dx + dy * dy <
                (gs->boomerang.radius + BOSS_RADIUS) *
                (gs->boomerang.radius + BOSS_RADIUS)) {
                weapon_hit_boss(gs, gs->boomerang.damage);
            }
        }
    }
}

void boomerang_draw(const GameState *gs, float scale, Vector2 offset) {
    if (!gs->boomerang.has) return;
    float t = (float)GetTime();
    for (int i = 0; i < MAX_BOOMERANGS; i++) {
        if (!gs->boomerang.slots[i].active) continue;
        float x = gs->boomerang.slots[i].pos.x * scale + offset.x;
        float y = gs->boomerang.slots[i].pos.y * scale + offset.y;
        float r = gs->boomerang.radius * scale;
        float spin = t * 12.0f;
        Color col = {150, 255, 180, 255};
        for (int k = 0; k < 2; k++) {
            float a = spin + (k * 3.14159f);
            Vector2 e1 = {x + cosf(a) * r, y + sinf(a) * r};
            Vector2 e2 = {x - cosf(a) * r, y - sinf(a) * r};
            DrawLineEx(e1, e2, 3.0f * scale, col);
        }
        DrawCircleV((Vector2){x, y}, r * 0.35f, (Color){220, 255, 220, 255});
    }
}
