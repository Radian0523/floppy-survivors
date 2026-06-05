#include "game.h"
#include <math.h>
#include <stdlib.h>

void enemy_spawn(GameState *gs) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!gs->enemies[i].active) {
            Enemy *e = &gs->enemies[i];
            e->active = true;

            int side = rand() % 4;
            switch (side) {
                case 0: // top
                    e->pos.x = rand() % LOGICAL_W;
                    e->pos.y = -SPAWN_MARGIN;
                    break;
                case 1: // bottom
                    e->pos.x = rand() % LOGICAL_W;
                    e->pos.y = LOGICAL_H + SPAWN_MARGIN;
                    break;
                case 2: // left
                    e->pos.x = -SPAWN_MARGIN;
                    e->pos.y = rand() % LOGICAL_H;
                    break;
                case 3: // right
                    e->pos.x = LOGICAL_W + SPAWN_MARGIN;
                    e->pos.y = rand() % LOGICAL_H;
                    break;
            }

            float time_bonus = gs->game_time / GAME_DURATION * ENEMY_SPEED_TIME_BONUS;
            float random_variance = ((rand() % 100) / 100.0f - 0.5f) * 10.0f;
            e->speed = ENEMY_BASE_SPEED + time_bonus + random_variance;

            int hp_bonus = (int)(gs->game_time / 60.0f);
            e->hp = ENEMY_BASE_HP + hp_bonus;

            return;
        }
    }
}

void enemy_update(GameState *gs, float dt) {
    gs->spawn_timer -= dt;
    if (gs->spawn_timer <= 0) {
        float progress = gs->game_time / GAME_DURATION;
        gs->spawn_interval = SPAWN_INTERVAL_INITIAL -
            progress * (SPAWN_INTERVAL_INITIAL - SPAWN_INTERVAL_MIN);
        if (gs->spawn_interval < SPAWN_INTERVAL_MIN)
            gs->spawn_interval = SPAWN_INTERVAL_MIN;

        gs->spawn_timer = gs->spawn_interval;

        int spawn_count = 1 + (int)(progress * 3);
        for (int i = 0; i < spawn_count; i++) {
            enemy_spawn(gs);
        }
    }

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!gs->enemies[i].active) continue;

        Enemy *e = &gs->enemies[i];
        float dx = gs->player.pos.x - e->pos.x;
        float dy = gs->player.pos.y - e->pos.y;
        float len = sqrtf(dx * dx + dy * dy);

        if (len > 0) {
            e->vel.x = (dx / len) * e->speed;
            e->vel.y = (dy / len) * e->speed;
        }

        e->pos.x += e->vel.x * dt;
        e->pos.y += e->vel.y * dt;

        float dist = sqrtf(dx * dx + dy * dy);
        if (dist < PLAYER_RADIUS + ENEMY_RADIUS) {
            player_take_damage(&gs->player, ENEMY_DAMAGE);
        }
    }
}

void enemy_draw(const Enemy enemies[], float scale, Vector2 offset) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        float x = enemies[i].pos.x * scale + offset.x;
        float y = enemies[i].pos.y * scale + offset.y;
        float r = ENEMY_RADIUS * scale;
        DrawCircleV((Vector2){x, y}, r, (Color){255, 50, 100, 255});
        DrawCircleV((Vector2){x, y}, r * 0.6f, (Color){255, 100, 150, 200});
    }
}
