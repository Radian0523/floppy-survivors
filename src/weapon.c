#include "game.h"
#include <math.h>
#include <float.h>

static Color enemy_color_for_type(EnemyType type) {
    switch (type) {
        case ENEMY_BIT:       return (Color){255, 50, 100, 255};
        case ENEMY_FRAGMENT:  return (Color){255, 180, 60, 255};
        case ENEMY_PACKET:    return (Color){255, 120, 40, 255};
        case ENEMY_GLITCH:    return (Color){220, 80, 180, 255};
        case ENEMY_SPLITTER:  return (Color){255, 80, 60, 255};
        case ENEMY_BOMBER:    return (Color){255, 60, 140, 255};
        case ENEMY_RANGER:    return (Color){255, 100, 180, 255};
        case ENEMY_SWARM:     return (Color){255, 200, 180, 255};
        case ENEMY_BADSECTOR: return (Color){200, 60, 60, 255};
        case ENEMY_PHASER:    return (Color){230, 100, 100, 255};
        case ENEMY_TRACKER:   return (Color){255, 90, 70, 255};
        default:              return (Color){255, 200, 100, 255};
    }
}

static void kill_enemy(GameState *gs, int idx) {
    Enemy *e = &gs->enemies[idx];
    Vector2 pos = e->pos;
    EnemyType type = e->type;
    bool was_elite = e->is_elite;

    int burst = was_elite ? 30 : 10;
    particles_spawn_burst(gs, pos, enemy_color_for_type(type), burst);
    shake_add(gs, was_elite ? SHAKE_BOSS_HIT : SHAKE_KILL);

    gem_spawn(gs, pos);
    if (was_elite) {
        chest_drop(gs, pos);
        // Bonus gems around the elite
        for (int i = 0; i < 5; i++) {
            float a = (2.0f * 3.14159f * i) / 5.0f;
            Vector2 gp = {pos.x + cosf(a) * 18.0f, pos.y + sinf(a) * 18.0f};
            gem_spawn(gs, gp);
        }
    } else {
        item_drop_roll(gs, type, pos);
    }
    e->active = false;
    gs->kills++;
    audio_play(SFX_ENEMY_DIE);

    if (type == ENEMY_SPLITTER) {
        for (int i = 0; i < SPLITTER_CHILD_COUNT; i++) {
            float angle = (2.0f * 3.14159f * i) / SPLITTER_CHILD_COUNT;
            Vector2 child_pos = {
                pos.x + cosf(angle) * 15.0f,
                pos.y + sinf(angle) * 15.0f
            };
            enemy_spawn_at(gs, ENEMY_SPLITTER_CHILD, child_pos);
        }
    } else if (type == ENEMY_BOMBER) {
        particles_spawn_burst(gs, pos, (Color){255, 100, 200, 255}, 24);
        shake_add(gs, SHAKE_HIT);
        float dx = gs->player.pos.x - pos.x;
        float dy = gs->player.pos.y - pos.y;
        if (sqrtf(dx * dx + dy * dy) < BOMBER_EXPLOSION_RADIUS + PLAYER_RADIUS) {
            player_take_damage(gs, BOMBER_EXPLOSION_DAMAGE);
        }
    }
}

static int find_nearest_enemy(const GameState *gs, Vector2 *out_pos) {
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
            if (out_pos) *out_pos = gs->enemies[i].pos;
        }
    }

    if (gs->boss.active) {
        float dx = gs->boss.pos.x - gs->player.pos.x;
        float dy = gs->boss.pos.y - gs->player.pos.y;
        float dist = dx * dx + dy * dy;
        if (dist < min_dist) {
            nearest = -2;
            if (out_pos) *out_pos = gs->boss.pos;
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
    if (!gs->has_pulse_bolt) return;

    gs->fire_timer -= dt;
    if (gs->fire_timer <= 0) {
        gs->fire_timer = gs->fire_interval;

        Vector2 target_pos;
        int target = find_nearest_enemy(gs, &target_pos);
        if (target != -1) {
            float dx = target_pos.x - gs->player.pos.x;
            float dy = target_pos.y - gs->player.pos.y;
            float len = sqrtf(dx * dx + dy * dy);
            if (len > 0) {
                Vector2 dir = {dx / len, dy / len};
                for (int i = 0; i < gs->bullet_count; i++) {
                    fire_bullet(gs, dir);
                }
                audio_play(SFX_SHOOT);
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

        if (gs->boss.active) {
            float dx = b->pos.x - gs->boss.pos.x;
            float dy = b->pos.y - gs->boss.pos.y;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist < BULLET_RADIUS + BOSS_RADIUS) {
                boss_take_damage(gs, gs->bullet_damage);
                b->active = false;
                continue;
            }
        }

        for (int j = 0; j < MAX_ENEMIES; j++) {
            if (!gs->enemies[j].active) continue;
            if (gs->enemies[j].phased) continue;
            float dx = b->pos.x - gs->enemies[j].pos.x;
            float dy = b->pos.y - gs->enemies[j].pos.y;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist < BULLET_RADIUS + gs->enemies[j].radius) {
                gs->enemies[j].hp -= gs->bullet_damage;
                popup_spawn(gs, gs->enemies[j].pos, gs->bullet_damage,
                    (Color){255, 255, 200, 255});
                b->active = false;
                if (gs->enemies[j].hp <= 0) {
                    kill_enemy(gs, j);
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

        float vx = bullets[i].vel.x;
        float vy = bullets[i].vel.y;
        float vlen = sqrtf(vx * vx + vy * vy);
        if (vlen < 0.001f) vlen = 1;
        float dx = vx / vlen;
        float dy = vy / vlen;

        float trail_len = r * 4.0f;
        Vector2 tail = {x - dx * trail_len, y - dy * trail_len};
        Vector2 head = {x + dx * r * 0.5f, y + dy * r * 0.5f};

        DrawLineEx(tail, head, r * 1.6f, (Color){255, 255, 100, 180});
        DrawLineEx(tail, head, r * 0.8f, (Color){255, 255, 220, 255});
        DrawCircleV((Vector2){x, y}, r * 0.5f, (Color){255, 255, 255, 255});
    }
}

// === ORBITERS ===

void orbiters_init(GameState *gs) {
    gs->has_orbiters = true;
    gs->orbiter_count = ORBITER_COUNT_BASE;
    gs->orbiter_damage = ORBITER_DAMAGE;
    gs->orbiter_orbit_radius = ORBITER_ORBIT_RADIUS;

    for (int i = 0; i < MAX_ORBITERS; i++) {
        gs->orbiters[i].active = (i < gs->orbiter_count);
        gs->orbiters[i].angle = (2.0f * 3.14159f * i) / gs->orbiter_count;
    }
}

void orbiters_update(GameState *gs, float dt) {
    if (!gs->has_orbiters) return;

    for (int i = 0; i < gs->orbiter_count; i++) {
        if (!gs->orbiters[i].active) continue;

        gs->orbiters[i].angle += ORBITER_SPEED * dt;
        if (gs->orbiters[i].angle > 2.0f * 3.14159f)
            gs->orbiters[i].angle -= 2.0f * 3.14159f;

        float ox = gs->player.pos.x + cosf(gs->orbiters[i].angle) * gs->orbiter_orbit_radius;
        float oy = gs->player.pos.y + sinf(gs->orbiters[i].angle) * gs->orbiter_orbit_radius;

        for (int j = 0; j < MAX_ENEMIES; j++) {
            if (!gs->enemies[j].active) continue;
            if (gs->enemies[j].phased) continue;
            float dx = ox - gs->enemies[j].pos.x;
            float dy = oy - gs->enemies[j].pos.y;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist < ORBITER_RADIUS + gs->enemies[j].radius) {
                gs->enemies[j].hp -= gs->orbiter_damage;
                popup_spawn(gs, gs->enemies[j].pos, gs->orbiter_damage,
                    (Color){200, 220, 255, 255});
                if (gs->enemies[j].hp <= 0) {
                    kill_enemy(gs, j);
                }
            }
        }

        if (gs->boss.active) {
            float dx = ox - gs->boss.pos.x;
            float dy = oy - gs->boss.pos.y;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist < ORBITER_RADIUS + BOSS_RADIUS) {
                boss_take_damage(gs, gs->orbiter_damage);
            }
        }
    }
}

void orbiters_draw(const GameState *gs, float scale, Vector2 offset) {
    if (!gs->has_orbiters) return;

    for (int i = 0; i < gs->orbiter_count; i++) {
        if (!gs->orbiters[i].active) continue;

        float ox = gs->player.pos.x + cosf(gs->orbiters[i].angle) * gs->orbiter_orbit_radius;
        float oy = gs->player.pos.y + sinf(gs->orbiters[i].angle) * gs->orbiter_orbit_radius;

        float x = ox * scale + offset.x;
        float y = oy * scale + offset.y;
        float r = ORBITER_RADIUS * scale;

        DrawCircleV((Vector2){x, y}, r, (Color){100, 150, 255, 255});
        DrawCircleV((Vector2){x, y}, r * 0.6f, (Color){150, 200, 255, 200});
    }
}

// === BEAM ===

void beam_init(GameState *gs) {
    gs->has_beam = true;
    gs->beam.angle = 0;
    gs->beam.timer = BEAM_INTERVAL;
    gs->beam.firing = false;
    gs->beam.fire_timer = 0;
    gs->beam_interval = BEAM_INTERVAL;
    gs->beam_damage = BEAM_DAMAGE;
    gs->beam_length = BEAM_LENGTH;
    gs->beam_width = BEAM_WIDTH;
}

void beam_update(GameState *gs, float dt) {
    if (!gs->has_beam) return;

    if (gs->beam.firing) {
        gs->beam.fire_timer -= dt;
        if (gs->beam.fire_timer <= 0) {
            gs->beam.firing = false;
            gs->beam.timer = gs->beam_interval;
        }

        float progress = 1.0f - (gs->beam.fire_timer / BEAM_DURATION);
        gs->beam.angle = gs->beam.center_angle + BEAM_SWEEP_ANGLE - (progress * 2.0f * BEAM_SWEEP_ANGLE);

        float cos_a = cosf(gs->beam.angle);
        float sin_a = sinf(gs->beam.angle);

        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!gs->enemies[i].active) continue;
            if (gs->enemies[i].phased) continue;

            float dx = gs->enemies[i].pos.x - gs->player.pos.x;
            float dy = gs->enemies[i].pos.y - gs->player.pos.y;

            float proj = dx * cos_a + dy * sin_a;
            if (proj < 0 || proj > gs->beam_length) continue;

            float perp = fabsf(-dx * sin_a + dy * cos_a);
            if (perp < gs->beam_width / 2 + gs->enemies[i].radius) {
                gs->enemies[i].hp -= gs->beam_damage;
                popup_spawn(gs, gs->enemies[i].pos, gs->beam_damage,
                    (Color){255, 150, 150, 255});
                if (gs->enemies[i].hp <= 0) {
                    kill_enemy(gs, i);
                }
            }
        }

        if (gs->boss.active) {
            float dx = gs->boss.pos.x - gs->player.pos.x;
            float dy = gs->boss.pos.y - gs->player.pos.y;
            float proj = dx * cos_a + dy * sin_a;
            if (proj >= 0 && proj <= gs->beam_length) {
                float perp = fabsf(-dx * sin_a + dy * cos_a);
                if (perp < gs->beam_width / 2 + BOSS_RADIUS) {
                    boss_take_damage(gs, gs->beam_damage);
                }
            }
        }
    } else {
        gs->beam.timer -= dt;
        if (gs->beam.timer <= 0) {
            gs->beam.firing = true;
            gs->beam.fire_timer = BEAM_DURATION;
            gs->beam.center_angle = gs->player.facing_angle;
            gs->beam.angle = gs->beam.center_angle + BEAM_SWEEP_ANGLE;
            audio_play(SFX_BEAM);
        }
    }
}

void beam_draw(const GameState *gs, float scale, Vector2 offset) {
    if (!gs->has_beam || !gs->beam.firing) return;

    float px = gs->player.pos.x * scale + offset.x;
    float py = gs->player.pos.y * scale + offset.y;

    float ex = (gs->player.pos.x + cosf(gs->beam.angle) * gs->beam_length) * scale + offset.x;
    float ey = (gs->player.pos.y + sinf(gs->beam.angle) * gs->beam_length) * scale + offset.y;

    float w = gs->beam_width * scale;

    DrawLineEx((Vector2){px, py}, (Vector2){ex, ey}, w, (Color){180, 80, 255, 200});
    DrawLineEx((Vector2){px, py}, (Vector2){ex, ey}, w * 0.5f, (Color){220, 180, 255, 255});
}

// === NOVA ===

void nova_init(GameState *gs) {
    gs->has_nova = true;
    gs->nova.timer = NOVA_INTERVAL;
    gs->nova.current_radius = 0;
    gs->nova.expanding = false;
    gs->nova_interval = NOVA_INTERVAL;
    gs->nova_damage = NOVA_DAMAGE;
    gs->nova_max_radius = NOVA_RADIUS_BASE;
}

void nova_update(GameState *gs, float dt) {
    if (!gs->has_nova) return;

    if (gs->nova.expanding) {
        gs->nova.current_radius += NOVA_EXPAND_SPEED * dt;

        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!gs->enemies[i].active) continue;
            if (gs->enemies[i].phased) continue;

            float dx = gs->enemies[i].pos.x - gs->player.pos.x;
            float dy = gs->enemies[i].pos.y - gs->player.pos.y;
            float dist = sqrtf(dx * dx + dy * dy);

            float ring_inner = gs->nova.current_radius - 15.0f;
            float ring_outer = gs->nova.current_radius + 15.0f;

            if (dist > ring_inner && dist < ring_outer) {
                gs->enemies[i].hp -= gs->nova_damage;
                popup_spawn(gs, gs->enemies[i].pos, gs->nova_damage,
                    (Color){255, 200, 255, 255});
                if (gs->enemies[i].hp <= 0) {
                    kill_enemy(gs, i);
                }
            }
        }

        if (gs->boss.active) {
            float dx = gs->boss.pos.x - gs->player.pos.x;
            float dy = gs->boss.pos.y - gs->player.pos.y;
            float dist = sqrtf(dx * dx + dy * dy);
            float ring_inner = gs->nova.current_radius - 15.0f;
            float ring_outer = gs->nova.current_radius + 15.0f;
            if (dist > ring_inner - BOSS_RADIUS && dist < ring_outer + BOSS_RADIUS) {
                boss_take_damage(gs, gs->nova_damage);
            }
        }

        if (gs->nova.current_radius >= gs->nova_max_radius) {
            gs->nova.expanding = false;
            gs->nova.timer = gs->nova_interval;
        }
    } else {
        gs->nova.timer -= dt;
        if (gs->nova.timer <= 0) {
            gs->nova.expanding = true;
            gs->nova.current_radius = 0;
            audio_play(SFX_NOVA);
        }
    }
}

void nova_draw(const GameState *gs, float scale, Vector2 offset) {
    if (!gs->has_nova || !gs->nova.expanding) return;

    float px = gs->player.pos.x * scale + offset.x;
    float py = gs->player.pos.y * scale + offset.y;
    float r = gs->nova.current_radius * scale;

    float alpha = 1.0f - (gs->nova.current_radius / gs->nova_max_radius);
    Color col = {100, 255, 255, (unsigned char)(200 * alpha)};
    Color col_inner = {180, 255, 255, (unsigned char)(255 * alpha)};

    DrawRing((Vector2){px, py}, r - 5 * scale, r + 5 * scale, 0, 360, 36, col);
    DrawRing((Vector2){px, py}, r - 2 * scale, r + 2 * scale, 0, 360, 36, col_inner);
}
