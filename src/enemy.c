#include "game.h"
#include <math.h>
#include <stdlib.h>

static Vector2 random_spawn_pos(void) {
    Vector2 pos;
    int side = rand() % 4;
    switch (side) {
        case 0:
            pos.x = rand() % LOGICAL_W;
            pos.y = -SPAWN_MARGIN;
            break;
        case 1:
            pos.x = rand() % LOGICAL_W;
            pos.y = LOGICAL_H + SPAWN_MARGIN;
            break;
        case 2:
            pos.x = -SPAWN_MARGIN;
            pos.y = rand() % LOGICAL_H;
            break;
        default:
            pos.x = LOGICAL_W + SPAWN_MARGIN;
            pos.y = rand() % LOGICAL_H;
            break;
    }
    return pos;
}

static void init_enemy_stats(Enemy *e, EnemyType type, float time_bonus) {
    e->type = type;
    e->glitch_timer = 0;

    switch (type) {
        case ENEMY_BIT:
            e->hp = BIT_HP;
            e->speed = BIT_SPEED + time_bonus;
            e->radius = BIT_RADIUS;
            break;
        case ENEMY_FRAGMENT:
            e->hp = FRAGMENT_HP;
            e->speed = FRAGMENT_SPEED + time_bonus * 0.5f;
            e->radius = FRAGMENT_RADIUS;
            break;
        case ENEMY_PACKET:
            e->hp = PACKET_HP + (int)(time_bonus / 10.0f);
            e->speed = PACKET_SPEED + time_bonus * 0.3f;
            e->radius = PACKET_RADIUS;
            break;
        case ENEMY_GLITCH:
            e->hp = GLITCH_HP;
            e->speed = GLITCH_SPEED + time_bonus * 0.5f;
            e->radius = GLITCH_RADIUS;
            e->glitch_timer = GLITCH_DIR_CHANGE_TIME;
            break;
        case ENEMY_SPLITTER:
            e->hp = SPLITTER_HP;
            e->speed = SPLITTER_SPEED + time_bonus * 0.4f;
            e->radius = SPLITTER_RADIUS;
            break;
        case ENEMY_SPLITTER_CHILD:
            e->hp = 1;
            e->speed = FRAGMENT_SPEED;
            e->radius = FRAGMENT_RADIUS;
            break;
        default:
            e->hp = BIT_HP;
            e->speed = BIT_SPEED;
            e->radius = BIT_RADIUS;
            break;
    }

    float variance = ((rand() % 100) / 100.0f - 0.5f) * 10.0f;
    e->speed += variance;
}

static EnemyType choose_enemy_type(float game_time) {
    float progress = game_time / GAME_DURATION;

    if (progress < 0.2f) {
        return ENEMY_BIT;
    } else if (progress < 0.4f) {
        int r = rand() % 100;
        if (r < 60) return ENEMY_BIT;
        if (r < 90) return ENEMY_FRAGMENT;
        return ENEMY_PACKET;
    } else if (progress < 0.6f) {
        int r = rand() % 100;
        if (r < 40) return ENEMY_BIT;
        if (r < 60) return ENEMY_FRAGMENT;
        if (r < 80) return ENEMY_PACKET;
        return ENEMY_GLITCH;
    } else {
        int r = rand() % 100;
        if (r < 25) return ENEMY_BIT;
        if (r < 40) return ENEMY_FRAGMENT;
        if (r < 55) return ENEMY_PACKET;
        if (r < 75) return ENEMY_GLITCH;
        return ENEMY_SPLITTER;
    }
}

void enemy_spawn(GameState *gs) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!gs->enemies[i].active) {
            Enemy *e = &gs->enemies[i];
            e->active = true;
            e->pos = random_spawn_pos();

            float time_bonus = gs->game_time / GAME_DURATION * ENEMY_SPEED_TIME_BONUS;
            EnemyType type = choose_enemy_type(gs->game_time);
            init_enemy_stats(e, type, time_bonus);

            return;
        }
    }
}

void enemy_spawn_at(GameState *gs, EnemyType type, Vector2 pos) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!gs->enemies[i].active) {
            Enemy *e = &gs->enemies[i];
            e->active = true;
            e->pos = pos;

            float time_bonus = gs->game_time / GAME_DURATION * ENEMY_SPEED_TIME_BONUS;
            init_enemy_stats(e, type, time_bonus);

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

        if (e->type == ENEMY_GLITCH) {
            e->glitch_timer -= dt;
            if (e->glitch_timer <= 0) {
                e->glitch_timer = GLITCH_DIR_CHANGE_TIME;
                float angle = ((rand() % 360) / 180.0f) * 3.14159f;
                e->vel.x = cosf(angle) * e->speed;
                e->vel.y = sinf(angle) * e->speed;
            }
        } else {
            if (len > 0) {
                e->vel.x = (dx / len) * e->speed;
                e->vel.y = (dy / len) * e->speed;
            }
        }

        e->pos.x += e->vel.x * dt;
        e->pos.y += e->vel.y * dt;

        float dist = sqrtf(dx * dx + dy * dy);
        if (dist < PLAYER_RADIUS + e->radius) {
            player_take_damage(&gs->player, ENEMY_DAMAGE);
        }
    }
}

static Color get_enemy_color(EnemyType type) {
    switch (type) {
        case ENEMY_BIT:
            return (Color){255, 50, 100, 255};
        case ENEMY_FRAGMENT:
            return (Color){100, 255, 150, 255};
        case ENEMY_PACKET:
            return (Color){255, 150, 50, 255};
        case ENEMY_GLITCH:
            return (Color){150, 100, 255, 255};
        case ENEMY_SPLITTER:
            return (Color){255, 255, 100, 255};
        case ENEMY_SPLITTER_CHILD:
            return (Color){255, 200, 100, 255};
        default:
            return (Color){255, 50, 100, 255};
    }
}

void enemy_draw(const Enemy enemies[], float scale, Vector2 offset) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;

        const Enemy *e = &enemies[i];
        float x = e->pos.x * scale + offset.x;
        float y = e->pos.y * scale + offset.y;
        float r = e->radius * scale;

        Color col = get_enemy_color(e->type);
        Color col_inner = {
            (unsigned char)(col.r * 0.7f + 80),
            (unsigned char)(col.g * 0.7f + 80),
            (unsigned char)(col.b * 0.7f + 80),
            200
        };

        if (e->type == ENEMY_GLITCH) {
            float jitter = ((rand() % 10) - 5) * 0.5f * scale;
            x += jitter;
            y += jitter;
        }

        if (e->type == ENEMY_PACKET) {
            DrawRectangle((int)(x - r), (int)(y - r), (int)(r * 2), (int)(r * 2), col);
            DrawRectangle((int)(x - r * 0.6f), (int)(y - r * 0.6f),
                (int)(r * 1.2f), (int)(r * 1.2f), col_inner);
        } else {
            DrawCircleV((Vector2){x, y}, r, col);
            DrawCircleV((Vector2){x, y}, r * 0.6f, col_inner);
        }
    }
}
