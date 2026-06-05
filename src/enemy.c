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
    e->type_timer = 0;
    e->phase_timer = 0;
    e->phased = false;

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
            e->type_timer = GLITCH_DIR_CHANGE_TIME;
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
        case ENEMY_BOMBER:
            e->hp = BOMBER_HP;
            e->speed = BOMBER_SPEED + time_bonus * 0.4f;
            e->radius = BOMBER_RADIUS;
            break;
        case ENEMY_RANGER:
            e->hp = RANGER_HP;
            e->speed = RANGER_SPEED + time_bonus * 0.3f;
            e->radius = RANGER_RADIUS;
            e->type_timer = RANGER_FIRE_INTERVAL * 0.6f;
            break;
        case ENEMY_SWARM:
            e->hp = SWARM_HP;
            e->speed = SWARM_SPEED + time_bonus * 0.4f;
            e->radius = SWARM_RADIUS;
            break;
        case ENEMY_BADSECTOR:
            e->hp = BADSECTOR_HP + (int)(time_bonus / 8.0f);
            e->speed = 0;
            e->radius = BADSECTOR_RADIUS;
            e->type_timer = BADSECTOR_FIRE_INTERVAL;
            break;
        case ENEMY_PHASER:
            e->hp = PHASER_HP;
            e->speed = PHASER_SPEED + time_bonus * 0.4f;
            e->radius = PHASER_RADIUS;
            e->phase_timer = 0;
            break;
        case ENEMY_TRACKER:
            e->hp = TRACKER_HP;
            e->speed = TRACKER_SPEED + time_bonus * 0.4f;
            e->radius = TRACKER_RADIUS;
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

    if (progress < 0.08f) {
        return ENEMY_BIT;
    } else if (progress < 0.18f) {
        int r = rand() % 100;
        if (r < 55) return ENEMY_BIT;
        if (r < 85) return ENEMY_FRAGMENT;
        return ENEMY_PACKET;
    } else if (progress < 0.30f) {
        int r = rand() % 100;
        if (r < 35) return ENEMY_BIT;
        if (r < 55) return ENEMY_FRAGMENT;
        if (r < 70) return ENEMY_PACKET;
        if (r < 85) return ENEMY_GLITCH;
        return ENEMY_SWARM;
    } else if (progress < 0.45f) {
        int r = rand() % 100;
        if (r < 20) return ENEMY_BIT;
        if (r < 35) return ENEMY_FRAGMENT;
        if (r < 50) return ENEMY_PACKET;
        if (r < 62) return ENEMY_GLITCH;
        if (r < 75) return ENEMY_SWARM;
        if (r < 85) return ENEMY_BOMBER;
        return ENEMY_TRACKER;
    } else if (progress < 0.65f) {
        int r = rand() % 100;
        if (r < 15) return ENEMY_BIT;
        if (r < 28) return ENEMY_FRAGMENT;
        if (r < 40) return ENEMY_PACKET;
        if (r < 50) return ENEMY_GLITCH;
        if (r < 60) return ENEMY_SWARM;
        if (r < 70) return ENEMY_BOMBER;
        if (r < 80) return ENEMY_RANGER;
        if (r < 88) return ENEMY_TRACKER;
        if (r < 95) return ENEMY_PHASER;
        return ENEMY_BADSECTOR;
    } else {
        int r = rand() % 100;
        if (r < 10) return ENEMY_BIT;
        if (r < 20) return ENEMY_FRAGMENT;
        if (r < 30) return ENEMY_PACKET;
        if (r < 38) return ENEMY_GLITCH;
        if (r < 50) return ENEMY_SWARM;
        if (r < 60) return ENEMY_BOMBER;
        if (r < 70) return ENEMY_RANGER;
        if (r < 78) return ENEMY_TRACKER;
        if (r < 86) return ENEMY_PHASER;
        if (r < 92) return ENEMY_SPLITTER;
        return ENEMY_BADSECTOR;
    }
}

void enemy_spawn(GameState *gs) {
    EnemyType type = choose_enemy_type(gs->game_time);
    float time_bonus = gs->game_time / GAME_DURATION * ENEMY_SPEED_TIME_BONUS;

    if (type == ENEMY_SWARM) {
        Vector2 base = random_spawn_pos();
        for (int n = 0; n < SWARM_GROUP_SIZE; n++) {
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (!gs->enemies[i].active) {
                    Enemy *e = &gs->enemies[i];
                    e->active = true;
                    e->pos.x = base.x + ((rand() % 30) - 15);
                    e->pos.y = base.y + ((rand() % 30) - 15);
                    init_enemy_stats(e, ENEMY_SWARM, time_bonus);
                    break;
                }
            }
        }
        return;
    }

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!gs->enemies[i].active) {
            Enemy *e = &gs->enemies[i];
            e->active = true;
            e->pos = random_spawn_pos();
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

static void update_one_enemy(GameState *gs, Enemy *e, float dt) {
    float dx = gs->player.pos.x - e->pos.x;
    float dy = gs->player.pos.y - e->pos.y;
    float dist = sqrtf(dx * dx + dy * dy);

    switch (e->type) {
        case ENEMY_GLITCH:
            e->type_timer -= dt;
            if (e->type_timer <= 0) {
                e->type_timer = GLITCH_DIR_CHANGE_TIME;
                float angle = ((rand() % 360) / 180.0f) * 3.14159f;
                e->vel.x = cosf(angle) * e->speed;
                e->vel.y = sinf(angle) * e->speed;
            }
            break;

        case ENEMY_RANGER:
            e->type_timer -= dt;
            if (dist > 0) {
                float target_dist = RANGER_KEEP_DIST;
                float move_dir = (dist < target_dist) ? -1.0f : 1.0f;
                e->vel.x = (dx / dist) * e->speed * move_dir;
                e->vel.y = (dy / dist) * e->speed * move_dir;
            }
            if (e->type_timer <= 0 && dist < 250) {
                e->type_timer = RANGER_FIRE_INTERVAL;
                if (dist > 0) {
                    Vector2 fire_dir = {dx / dist, dy / dist};
                    enemy_bullet_spawn(gs, e->pos, fire_dir);
                }
            }
            break;

        case ENEMY_BADSECTOR:
            e->vel.x = 0;
            e->vel.y = 0;
            e->type_timer -= dt;
            if (e->type_timer <= 0) {
                e->type_timer = BADSECTOR_FIRE_INTERVAL;
                for (int i = 0; i < 6; i++) {
                    float angle = (2.0f * 3.14159f * i) / 6.0f;
                    Vector2 d = {cosf(angle), sinf(angle)};
                    enemy_bullet_spawn(gs, e->pos, d);
                }
            }
            break;

        case ENEMY_PHASER:
            e->phase_timer += dt;
            if (e->phase_timer >= PHASER_CYCLE) e->phase_timer -= PHASER_CYCLE;
            e->phased = (e->phase_timer < PHASER_CYCLE * PHASER_PHASE_RATIO);
            if (dist > 0) {
                e->vel.x = (dx / dist) * e->speed;
                e->vel.y = (dy / dist) * e->speed;
            }
            break;

        case ENEMY_TRACKER: {
            Vector2 target = {
                gs->player.pos.x + gs->player.vel.x * TRACKER_LEAD_TIME,
                gs->player.pos.y + gs->player.vel.y * TRACKER_LEAD_TIME
            };
            float tdx = target.x - e->pos.x;
            float tdy = target.y - e->pos.y;
            float tlen = sqrtf(tdx * tdx + tdy * tdy);
            if (tlen > 0) {
                e->vel.x = (tdx / tlen) * e->speed;
                e->vel.y = (tdy / tlen) * e->speed;
            }
            break;
        }

        default:
            if (dist > 0) {
                e->vel.x = (dx / dist) * e->speed;
                e->vel.y = (dy / dist) * e->speed;
            }
            break;
    }

    e->pos.x += e->vel.x * dt;
    e->pos.y += e->vel.y * dt;

    if (e->phased) return;

    if (dist < PLAYER_RADIUS + e->radius) {
        player_take_damage(gs, ENEMY_DAMAGE);
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
        update_one_enemy(gs, &gs->enemies[i], dt);
    }
}

static Color get_enemy_color(EnemyType type) {
    switch (type) {
        case ENEMY_BIT:            return (Color){255, 50, 100, 255};
        case ENEMY_FRAGMENT:       return (Color){100, 255, 150, 255};
        case ENEMY_PACKET:         return (Color){255, 150, 50, 255};
        case ENEMY_GLITCH:         return (Color){150, 100, 255, 255};
        case ENEMY_SPLITTER:       return (Color){255, 255, 100, 255};
        case ENEMY_SPLITTER_CHILD: return (Color){255, 200, 100, 255};
        case ENEMY_BOMBER:         return (Color){255, 80, 200, 255};
        case ENEMY_RANGER:         return (Color){80, 220, 220, 255};
        case ENEMY_SWARM:          return (Color){220, 220, 255, 255};
        case ENEMY_BADSECTOR:      return (Color){200, 60, 60, 255};
        case ENEMY_PHASER:         return (Color){180, 100, 220, 255};
        case ENEMY_TRACKER:        return (Color){255, 120, 80, 255};
        default:                   return (Color){255, 50, 100, 255};
    }
}

static void draw_enemy(const Enemy *e, float scale, Vector2 offset) {
    float x = e->pos.x * scale + offset.x;
    float y = e->pos.y * scale + offset.y;
    float r = e->radius * scale;

    Color col = get_enemy_color(e->type);
    if (e->phased) {
        col.a = 80;
    }
    Color col_inner = {
        (unsigned char)(col.r * 0.7f + 80),
        (unsigned char)(col.g * 0.7f + 80),
        (unsigned char)(col.b * 0.7f + 80),
        (unsigned char)(200 * (col.a / 255.0f))
    };

    if (e->type == ENEMY_GLITCH) {
        float jitter = ((rand() % 10) - 5) * 0.5f * scale;
        x += jitter;
        y += jitter;
    }

    switch (e->type) {
        case ENEMY_PACKET:
            DrawRectangle((int)(x - r), (int)(y - r), (int)(r * 2), (int)(r * 2), col);
            DrawRectangle((int)(x - r * 0.6f), (int)(y - r * 0.6f),
                (int)(r * 1.2f), (int)(r * 1.2f), col_inner);
            break;

        case ENEMY_BADSECTOR:
            DrawRectangle((int)(x - r), (int)(y - r), (int)(r * 2), (int)(r * 2), col);
            DrawRectangleLines((int)(x - r * 1.2f), (int)(y - r * 1.2f),
                (int)(r * 2.4f), (int)(r * 2.4f), col);
            DrawCircleV((Vector2){x, y}, r * 0.5f, col_inner);
            break;

        case ENEMY_BOMBER: {
            float pulse = 1.0f + 0.15f * sinf((float)GetTime() * 8.0f);
            DrawCircleV((Vector2){x, y}, r * pulse, col);
            DrawCircleV((Vector2){x, y}, r * 0.5f * pulse,
                (Color){255, 255, 255, col.a});
            break;
        }

        case ENEMY_RANGER:
            DrawLineEx((Vector2){x - r, y}, (Vector2){x + r, y}, 3.0f * scale, col);
            DrawLineEx((Vector2){x, y - r}, (Vector2){x, y + r}, 3.0f * scale, col);
            DrawCircleV((Vector2){x, y}, r * 0.4f, col_inner);
            break;

        case ENEMY_TRACKER: {
            float a = atan2f(e->vel.y, e->vel.x);
            Vector2 tip = {x + cosf(a) * r * 1.1f, y + sinf(a) * r * 1.1f};
            Vector2 bl = {x + cosf(a + 2.4f) * r, y + sinf(a + 2.4f) * r};
            Vector2 br = {x + cosf(a - 2.4f) * r, y + sinf(a - 2.4f) * r};
            DrawTriangle(tip, bl, br, col);
            DrawTriangle(tip, br, bl, col);
            break;
        }

        default:
            DrawCircleV((Vector2){x, y}, r, col);
            DrawCircleV((Vector2){x, y}, r * 0.6f, col_inner);
            break;
    }
}

void enemy_draw(const Enemy enemies[], float scale, Vector2 offset) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        draw_enemy(&enemies[i], scale, offset);
    }
}

void enemy_bullet_spawn(GameState *gs, Vector2 pos, Vector2 dir) {
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (!gs->enemy_bullets[i].active) {
            gs->enemy_bullets[i].active = true;
            gs->enemy_bullets[i].pos = pos;
            gs->enemy_bullets[i].vel.x = dir.x * ENEMY_BULLET_SPEED;
            gs->enemy_bullets[i].vel.y = dir.y * ENEMY_BULLET_SPEED;
            return;
        }
    }
}

void enemy_bullets_update(GameState *gs, float dt) {
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (!gs->enemy_bullets[i].active) continue;
        EnemyBullet *b = &gs->enemy_bullets[i];
        b->pos.x += b->vel.x * dt;
        b->pos.y += b->vel.y * dt;

        if (b->pos.x < -50 || b->pos.x > LOGICAL_W + 50 ||
            b->pos.y < -50 || b->pos.y > LOGICAL_H + 50) {
            b->active = false;
            continue;
        }

        float dx = b->pos.x - gs->player.pos.x;
        float dy = b->pos.y - gs->player.pos.y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist < ENEMY_BULLET_RADIUS + PLAYER_RADIUS) {
            player_take_damage(gs, ENEMY_BULLET_DAMAGE);
            b->active = false;
        }
    }
}

void enemy_bullets_draw(const GameState *gs, float scale, Vector2 offset) {
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (!gs->enemy_bullets[i].active) continue;
        float x = gs->enemy_bullets[i].pos.x * scale + offset.x;
        float y = gs->enemy_bullets[i].pos.y * scale + offset.y;
        float r = ENEMY_BULLET_RADIUS * scale;
        DrawCircleV((Vector2){x, y}, r, (Color){255, 80, 80, 255});
        DrawCircleV((Vector2){x, y}, r * 0.5f, (Color){255, 200, 200, 200});
    }
}
