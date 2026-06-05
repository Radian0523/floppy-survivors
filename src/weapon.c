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
            float dx = ox - gs->enemies[j].pos.x;
            float dy = oy - gs->enemies[j].pos.y;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist < ORBITER_RADIUS + ENEMY_RADIUS) {
                gs->enemies[j].hp -= gs->orbiter_damage;
                if (gs->enemies[j].hp <= 0) {
                    gem_spawn(gs, gs->enemies[j].pos);
                    gs->enemies[j].active = false;
                    gs->kills++;
                }
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

            float dx = gs->enemies[i].pos.x - gs->player.pos.x;
            float dy = gs->enemies[i].pos.y - gs->player.pos.y;

            float proj = dx * cos_a + dy * sin_a;
            if (proj < 0 || proj > gs->beam_length) continue;

            float perp = fabsf(-dx * sin_a + dy * cos_a);
            if (perp < BEAM_WIDTH / 2 + ENEMY_RADIUS) {
                gs->enemies[i].hp -= gs->beam_damage;
                if (gs->enemies[i].hp <= 0) {
                    gem_spawn(gs, gs->enemies[i].pos);
                    gs->enemies[i].active = false;
                    gs->kills++;
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
        }
    }
}

void beam_draw(const GameState *gs, float scale, Vector2 offset) {
    if (!gs->has_beam || !gs->beam.firing) return;

    float px = gs->player.pos.x * scale + offset.x;
    float py = gs->player.pos.y * scale + offset.y;

    float ex = (gs->player.pos.x + cosf(gs->beam.angle) * gs->beam_length) * scale + offset.x;
    float ey = (gs->player.pos.y + sinf(gs->beam.angle) * gs->beam_length) * scale + offset.y;

    float w = BEAM_WIDTH * scale;

    DrawLineEx((Vector2){px, py}, (Vector2){ex, ey}, w, (Color){255, 50, 50, 200});
    DrawLineEx((Vector2){px, py}, (Vector2){ex, ey}, w * 0.5f, (Color){255, 150, 150, 255});
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

            float dx = gs->enemies[i].pos.x - gs->player.pos.x;
            float dy = gs->enemies[i].pos.y - gs->player.pos.y;
            float dist = sqrtf(dx * dx + dy * dy);

            float ring_inner = gs->nova.current_radius - 15.0f;
            float ring_outer = gs->nova.current_radius + 15.0f;

            if (dist > ring_inner && dist < ring_outer) {
                gs->enemies[i].hp -= gs->nova_damage;
                if (gs->enemies[i].hp <= 0) {
                    gem_spawn(gs, gs->enemies[i].pos);
                    gs->enemies[i].active = false;
                    gs->kills++;
                }
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
        }
    }
}

void nova_draw(const GameState *gs, float scale, Vector2 offset) {
    if (!gs->has_nova || !gs->nova.expanding) return;

    float px = gs->player.pos.x * scale + offset.x;
    float py = gs->player.pos.y * scale + offset.y;
    float r = gs->nova.current_radius * scale;

    float alpha = 1.0f - (gs->nova.current_radius / gs->nova_max_radius);
    Color col = {255, 100, 255, (unsigned char)(200 * alpha)};
    Color col_inner = {255, 150, 255, (unsigned char)(255 * alpha)};

    DrawRing((Vector2){px, py}, r - 5 * scale, r + 5 * scale, 0, 360, 36, col);
    DrawRing((Vector2){px, py}, r - 2 * scale, r + 2 * scale, 0, 360, 36, col_inner);
}
