#include "game.h"
#include <math.h>
#include <float.h>

static int find_nearest_enemy(const GameState *gs) {
    int nearest = -1;
    float min_dist = FLT_MAX;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!gs->enemies[i].active) continue;
        float dx = gs->enemies[i].pos.x - gs->player.pos.x;
        float dy = gs->enemies[i].pos.y - gs->player.pos.y;
        float dist = dx * dx + dy * dy;
        if (dist < min_dist) {
            min_dist = dist;
            nearest = i;
        }
    }
    return nearest;
}

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
    gs->fire_timer -= dt;
    if (gs->fire_timer <= 0) {
        gs->fire_timer = gs->fire_interval;

        int target = find_nearest_enemy(gs);
        if (target >= 0) {
            float dx = gs->enemies[target].pos.x - gs->player.pos.x;
            float dy = gs->enemies[target].pos.y - gs->player.pos.y;
            float len = sqrtf(dx * dx + dy * dy);
            if (len > 0) {
                Vector2 dir = {dx / len, dy / len};
                for (int i = 0; i < gs->bullet_count; i++) {
                    fire_bullet(gs, dir);
                }
            }
        }
    }
}

void bullet_update(GameState *gs, float dt) {
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

        for (int j = 0; j < MAX_ENEMIES; j++) {
            if (!gs->enemies[j].active) continue;
            float dx = b->pos.x - gs->enemies[j].pos.x;
            float dy = b->pos.y - gs->enemies[j].pos.y;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist < BULLET_RADIUS + ENEMY_RADIUS) {
                gs->enemies[j].hp -= gs->bullet_damage;
                b->active = false;
                if (gs->enemies[j].hp <= 0) {
                    gem_spawn(gs, gs->enemies[j].pos);
                    gs->enemies[j].active = false;
                    gs->kills++;
                }
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
        DrawCircleV((Vector2){x, y}, r, (Color){255, 255, 100, 255});
        DrawCircleV((Vector2){x, y}, r * 0.5f, (Color){255, 255, 200, 200});
    }
}
