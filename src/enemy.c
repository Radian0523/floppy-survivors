#include "game.h"
#include "weapon_util.h"
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
    e->is_elite = false;
    // Reset hit cooldown timers so a recycled slot doesn't inherit stale times.
    // Set negative so first hit always lands regardless of current game_time.
    for (int k = 0; k < (int)(sizeof(e->last_hit_time)/sizeof(e->last_hit_time[0])); k++) {
        e->last_hit_time[k] = -1000.0f;
    }

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
        case ENEMY_SWARM:
            e->hp = SWARM_HP;
            e->speed = SWARM_SPEED + time_bonus * 0.4f;
            e->radius = SWARM_RADIUS;
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

    int scaled_hp = (int)(e->hp * g_params.enemy_hp_mult + 0.5f);
    if (scaled_hp < 1) scaled_hp = 1;
    e->hp = scaled_hp;
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
        if (r < 17) return ENEMY_BIT;
        if (r < 32) return ENEMY_FRAGMENT;
        if (r < 47) return ENEMY_PACKET;
        if (r < 60) return ENEMY_GLITCH;
        if (r < 73) return ENEMY_SWARM;
        if (r < 85) return ENEMY_BOMBER;
        if (r < 92) return ENEMY_TRACKER;
        return ENEMY_PHASER;
    } else {
        int r = rand() % 100;
        if (r < 12) return ENEMY_BIT;
        if (r < 24) return ENEMY_FRAGMENT;
        if (r < 36) return ENEMY_PACKET;
        if (r < 46) return ENEMY_GLITCH;
        if (r < 60) return ENEMY_SWARM;
        if (r < 72) return ENEMY_BOMBER;
        if (r < 84) return ENEMY_TRACKER;
        if (r < 92) return ENEMY_PHASER;
        return ENEMY_SPLITTER;
    }
}

void enemy_spawn(GameState *gs) {
    EnemyType type = choose_enemy_type(gs->game_time);
    float time_bonus = gs->game_time / GAME_DURATION *
        ENEMY_SPEED_TIME_BONUS * g_params.enemy_speed_bonus_mult;

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

static void spawn_elite(GameState *gs) {
    // Pick an enemy type appropriate for time (medium/heavy)
    EnemyType candidates[] = {
        ENEMY_PACKET, ENEMY_GLITCH, ENEMY_BOMBER,
        ENEMY_PHASER, ENEMY_TRACKER, ENEMY_SPLITTER
    };
    int n = (int)(sizeof(candidates) / sizeof(candidates[0]));
    EnemyType type = candidates[rand() % n];

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!gs->enemies[i].active) {
            Enemy *e = &gs->enemies[i];
            e->active = true;
            e->pos = random_spawn_pos();
            float time_bonus = gs->game_time / GAME_DURATION * ENEMY_SPEED_TIME_BONUS;
            init_enemy_stats(e, type, time_bonus);
            e->is_elite = true;
            e->hp *= ELITE_HP_MULT;
            e->radius *= ELITE_RADIUS_MULT;
            audio_play(SFX_BOSS_SPAWN);
            return;
        }
    }
}

void enemy_spawn_elite_force(GameState *gs) {
    spawn_elite(gs);
}

void enemy_spawn_formation_force(GameState *gs);

static void spawn_formation(GameState *gs) {
    int kind = rand() % 2;
    float time_bonus = gs->game_time / GAME_DURATION * ENEMY_SPEED_TIME_BONUS;
    EnemyType type = choose_enemy_type(gs->game_time);

    if (kind == 0) {
        // Ring around player
        Vector2 c = gs->player.pos;
        for (int i = 0; i < FORMATION_RING_COUNT; i++) {
            float ang = (2.0f * 3.14159f * i) / FORMATION_RING_COUNT;
            Vector2 pos = {
                c.x + cosf(ang) * FORMATION_RING_RADIUS,
                c.y + sinf(ang) * FORMATION_RING_RADIUS
            };
            for (int j = 0; j < MAX_ENEMIES; j++) {
                if (!gs->enemies[j].active) {
                    Enemy *e = &gs->enemies[j];
                    e->active = true;
                    e->pos = pos;
                    init_enemy_stats(e, type, time_bonus);
                    break;
                }
            }
        }
    } else {
        // Line from one side
        int side = rand() % 4;
        for (int i = 0; i < FORMATION_LINE_COUNT; i++) {
            Vector2 pos;
            float t = (float)i / (FORMATION_LINE_COUNT - 1);
            switch (side) {
                case 0:
                    pos.x = LOGICAL_W * t;
                    pos.y = -SPAWN_MARGIN;
                    break;
                case 1:
                    pos.x = LOGICAL_W * t;
                    pos.y = LOGICAL_H + SPAWN_MARGIN;
                    break;
                case 2:
                    pos.x = -SPAWN_MARGIN;
                    pos.y = LOGICAL_H * t;
                    break;
                default:
                    pos.x = LOGICAL_W + SPAWN_MARGIN;
                    pos.y = LOGICAL_H * t;
                    break;
            }
            for (int j = 0; j < MAX_ENEMIES; j++) {
                if (!gs->enemies[j].active) {
                    Enemy *e = &gs->enemies[j];
                    e->active = true;
                    e->pos = pos;
                    init_enemy_stats(e, ENEMY_FRAGMENT, time_bonus);
                    break;
                }
            }
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
        // Find enemy index for retaliation
        int idx = (int)(e - gs->enemies);
        if (player_take_damage(gs, ENEMY_DAMAGE)) {
            weapon_hit_enemy(gs, idx, PLAYER_CONTACT_DAMAGE,
                             (Color){120, 220, 255, 255}, WEAPON_ID_COUNT);
        }
    }
}

void enemy_spawn_formation_force(GameState *gs) {
    spawn_formation(gs);
}

void enemy_update(GameState *gs, float dt) {
    gs->spawn_timer -= dt;
    if (gs->spawn_timer <= 0) {
        float progress = gs->game_time / GAME_DURATION;
        float spawn_min = SPAWN_INTERVAL_MIN * g_params.enemy_spawn_min_mult;
        gs->spawn_interval = SPAWN_INTERVAL_INITIAL -
            progress * (SPAWN_INTERVAL_INITIAL - spawn_min);
        if (gs->spawn_interval < spawn_min)
            gs->spawn_interval = spawn_min;

        gs->spawn_timer = gs->spawn_interval;

        int base_count = 1 + (int)(progress * 3);
        int spawn_count = (int)(base_count * g_params.spawn_count_mult + 0.5f);
        if (spawn_count < 1) spawn_count = 1;
        for (int i = 0; i < spawn_count; i++) {
            enemy_spawn(gs);
        }
    }

    if (gs->elite_timer > 0) gs->elite_timer -= dt;
    if (gs->elite_timer <= 0 && gs->game_time >= ELITE_FIRST_TIME) {
        spawn_elite(gs);
        gs->elite_timer = ELITE_INTERVAL;
    }

    if (gs->formation_timer > 0) gs->formation_timer -= dt;
    if (gs->formation_timer <= 0 && gs->game_time >= FORMATION_FIRST_TIME) {
        spawn_formation(gs);
        gs->formation_timer = FORMATION_INTERVAL;
    }

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!gs->enemies[i].active) continue;
        update_one_enemy(gs, &gs->enemies[i], dt);
    }
}

static Color get_enemy_color(EnemyType type) {
    switch (type) {
        case ENEMY_BIT:            return (Color){255, 50, 100, 255};   // red-pink
        case ENEMY_FRAGMENT:       return (Color){255, 180, 60, 255};   // bright orange
        case ENEMY_PACKET:         return (Color){255, 120, 40, 255};   // deep orange
        case ENEMY_GLITCH:         return (Color){220, 80, 180, 255};   // magenta-pink
        case ENEMY_SPLITTER:       return (Color){255, 80, 60, 255};    // scarlet
        case ENEMY_SPLITTER_CHILD: return (Color){255, 140, 80, 255};   // light scarlet
        case ENEMY_BOMBER:         return (Color){255, 60, 140, 255};   // pink
        case ENEMY_SWARM:          return (Color){255, 200, 180, 255};  // peach
        case ENEMY_PHASER:         return (Color){230, 100, 100, 255};  // muted red
        case ENEMY_TRACKER:        return (Color){255, 90, 70, 255};    // orange-red
        default:                   return (Color){255, 50, 100, 255};
    }
}

static void draw_enemy(const Enemy *e, float scale, Vector2 offset) {
    float x = e->pos.x * scale + offset.x;
    float y = e->pos.y * scale + offset.y;
    float r = e->radius * scale;

    if (e->is_elite) {
        float pulse = 0.6f + 0.4f * sinf((float)GetTime() * 6.0f);
        Color aura = {255, 220, 100, (unsigned char)(120 * pulse)};
        DrawCircleV((Vector2){x, y}, r * 1.6f, aura);
        DrawRing((Vector2){x, y}, r * 1.35f, r * 1.5f, 0, 360, 24,
            (Color){255, 230, 130, (unsigned char)(180 * pulse)});
        // Crown marker
        float cy = y - r * 1.7f;
        for (int k = -1; k <= 1; k++) {
            Vector2 tip = {x + k * r * 0.3f, cy};
            Vector2 bl = {x + (k - 0.3f) * r * 0.3f, cy + r * 0.3f};
            Vector2 br = {x + (k + 0.3f) * r * 0.3f, cy + r * 0.3f};
            DrawTriangle(tip, bl, br, (Color){255, 230, 130, 230});
        }
    }

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

        case ENEMY_BOMBER: {
            float pulse = 1.0f + 0.15f * sinf((float)GetTime() * 8.0f);
            DrawCircleV((Vector2){x, y}, r * pulse, col);
            DrawCircleV((Vector2){x, y}, r * 0.5f * pulse,
                (Color){255, 255, 255, col.a});
            break;
        }

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
    // Enemy bullets disabled: the small flying red orbs clutter mid/late game
    // and confuse the player about what is/isn't a threat. RANGER and
    // BADSECTOR fall back to being harmless distance/static obstacles.
    (void)gs; (void)pos; (void)dir;
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
    // Danmaku convention (Boghog / CAVE): bright core + dark border. Hue stays
    // in red/pink/magenta so bullets never clash with explosions (orange/gold)
    // or pickups (green/cyan). Slight pulse keeps motion saliency high.
    float t = (float)GetTime();
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (!gs->enemy_bullets[i].active) continue;
        float x = gs->enemy_bullets[i].pos.x * scale + offset.x;
        float y = gs->enemy_bullets[i].pos.y * scale + offset.y;
        float r = ENEMY_BULLET_RADIUS * scale;
        float pulse = 1.0f + 0.10f * sinf(t * 7.0f + i * 0.7f);

        // Dark outer border (value contrast: notch out of bright additive bg)
        DrawCircleV((Vector2){x, y}, r * 1.55f * pulse,
                    (Color){25, 0, 10, 220});
        // Saturated mid ring (the "this is enemy bullet" hue signal)
        DrawCircleV((Vector2){x, y}, r * 1.10f * pulse,
                    (Color){255, 70, 130, 255});
        // Bright pink core (highest luminance = visible against any color)
        DrawCircleV((Vector2){x, y}, r * 0.55f * pulse,
                    (Color){255, 220, 240, 255});
    }
}
