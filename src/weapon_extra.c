#include "game.h"
#include <math.h>
#include <stdlib.h>
#include <float.h>

// === Helpers ===

static int find_nearest_enemy_idx(const GameState *gs, Vector2 from, float max_dist) {
    int nearest = -1;
    float md = max_dist * max_dist;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!gs->enemies[i].active) continue;
        if (gs->enemies[i].phased) continue;
        float dx = gs->enemies[i].pos.x - from.x;
        float dy = gs->enemies[i].pos.y - from.y;
        float d2 = dx * dx + dy * dy;
        if (d2 < md) {
            md = d2;
            nearest = i;
        }
    }
    return nearest;
}

static void damage_enemy_at(GameState *gs, int idx, int dmg) {
    if (idx < 0 || idx >= MAX_ENEMIES) return;
    Enemy *e = &gs->enemies[idx];
    if (!e->active) return;
    e->hp -= dmg;
    popup_spawn(gs, e->pos, dmg, (Color){200, 255, 200, 255});
    if (e->hp <= 0) {
        particles_spawn_burst(gs, e->pos, (Color){200, 255, 200, 255}, 8);
        shake_add(gs, SHAKE_KILL);
        gem_spawn(gs, e->pos);
        e->active = false;
        gs->kills++;
        audio_play(SFX_ENEMY_DIE);
    }
}

static void aoe_damage(GameState *gs, Vector2 center, float radius, int dmg, Color col) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!gs->enemies[i].active) continue;
        if (gs->enemies[i].phased) continue;
        float dx = gs->enemies[i].pos.x - center.x;
        float dy = gs->enemies[i].pos.y - center.y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist < radius + gs->enemies[i].radius) {
            damage_enemy_at(gs, i, dmg);
        }
    }
    if (gs->boss.active) {
        float dx = gs->boss.pos.x - center.x;
        float dy = gs->boss.pos.y - center.y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist < radius + BOSS_RADIUS) {
            boss_take_damage(gs, dmg);
        }
    }
    particles_spawn_burst(gs, center, col, 12);
}

// === SPARK MINES ===

void mines_init(GameState *gs) {
    gs->has_mines = true;
    gs->mine_timer = MINE_INTERVAL;
    gs->mine_interval = MINE_INTERVAL;
    gs->mine_damage = MINE_DAMAGE;
    for (int i = 0; i < MAX_MINES; i++) gs->mines[i].active = false;
}

static void deploy_mine(GameState *gs, Vector2 pos) {
    for (int i = 0; i < MAX_MINES; i++) {
        if (!gs->mines[i].active) {
            gs->mines[i].active = true;
            gs->mines[i].pos = pos;
            gs->mines[i].life = MINE_LIFE;
            return;
        }
    }
}

void mines_update(GameState *gs, float dt) {
    if (!gs->has_mines) return;

    gs->mine_timer -= dt;
    if (gs->mine_timer <= 0) {
        gs->mine_timer = gs->mine_interval;
        deploy_mine(gs, gs->player.pos);
    }

    for (int i = 0; i < MAX_MINES; i++) {
        if (!gs->mines[i].active) continue;
        Mine *m = &gs->mines[i];
        m->life -= dt;

        bool detonate = (m->life <= 0);

        for (int j = 0; j < MAX_ENEMIES && !detonate; j++) {
            if (!gs->enemies[j].active) continue;
            if (gs->enemies[j].phased) continue;
            float dx = gs->enemies[j].pos.x - m->pos.x;
            float dy = gs->enemies[j].pos.y - m->pos.y;
            if (dx * dx + dy * dy < (MINE_RADIUS + gs->enemies[j].radius) *
                                    (MINE_RADIUS + gs->enemies[j].radius)) {
                detonate = true;
            }
        }
        if (!detonate && gs->boss.active) {
            float dx = gs->boss.pos.x - m->pos.x;
            float dy = gs->boss.pos.y - m->pos.y;
            if (dx * dx + dy * dy < (MINE_RADIUS + BOSS_RADIUS) *
                                    (MINE_RADIUS + BOSS_RADIUS)) {
                detonate = true;
            }
        }

        if (detonate) {
            aoe_damage(gs, m->pos, MINE_EXPLOSION_RADIUS, gs->mine_damage,
                (Color){255, 220, 100, 255});
            m->active = false;
        }
    }
}

void mines_draw(const GameState *gs, float scale, Vector2 offset) {
    if (!gs->has_mines) return;
    float t = (float)GetTime();
    for (int i = 0; i < MAX_MINES; i++) {
        if (!gs->mines[i].active) continue;
        float x = gs->mines[i].pos.x * scale + offset.x;
        float y = gs->mines[i].pos.y * scale + offset.y;
        float r = MINE_RADIUS * scale;
        float pulse = 0.5f + 0.5f * sinf(t * 8.0f);
        Color outer = {255, 220, 100, (unsigned char)(150 + 100 * pulse)};
        Color inner = {255, 255, 200, 255};
        DrawCircleV((Vector2){x, y}, r, outer);
        DrawCircleV((Vector2){x, y}, r * 0.5f, inner);
        // cross marker
        DrawLineEx((Vector2){x - r, y - r}, (Vector2){x + r, y + r}, scale, outer);
        DrawLineEx((Vector2){x + r, y - r}, (Vector2){x - r, y + r}, scale, outer);
    }
}

// === CHAIN LIGHTNING ===

void chain_init(GameState *gs) {
    gs->has_chain = true;
    gs->chain_timer = CHAIN_INTERVAL;
    gs->chain_interval = CHAIN_INTERVAL;
    gs->chain_jumps = CHAIN_JUMPS;
    gs->chain_damage = CHAIN_DAMAGE;
    gs->chain_visual.count = 0;
    gs->chain_visual.life = 0;
}

void chain_update(GameState *gs, float dt) {
    if (!gs->has_chain) return;

    if (gs->chain_visual.life > 0) gs->chain_visual.life -= dt;

    gs->chain_timer -= dt;
    if (gs->chain_timer > 0) return;
    gs->chain_timer = gs->chain_interval;

    int first = find_nearest_enemy_idx(gs, gs->player.pos, CHAIN_RANGE);
    if (first < 0) return;

    gs->chain_visual.count = 0;
    gs->chain_visual.life = CHAIN_VISUAL_LIFE;
    gs->chain_visual.points[gs->chain_visual.count++] = gs->player.pos;

    bool hit[MAX_ENEMIES] = {false};
    int current = first;
    Vector2 cur_pos = gs->enemies[current].pos;

    for (int j = 0; j < gs->chain_jumps && current >= 0; j++) {
        hit[current] = true;
        gs->chain_visual.points[gs->chain_visual.count++] = cur_pos;
        damage_enemy_at(gs, current, gs->chain_damage);

        if (gs->chain_visual.count >= CHAIN_MAX_POINTS) break;

        int next = -1;
        float best = CHAIN_JUMP_RANGE * CHAIN_JUMP_RANGE;
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!gs->enemies[i].active) continue;
            if (gs->enemies[i].phased) continue;
            if (hit[i]) continue;
            float dx = gs->enemies[i].pos.x - cur_pos.x;
            float dy = gs->enemies[i].pos.y - cur_pos.y;
            float d2 = dx * dx + dy * dy;
            if (d2 < best) {
                best = d2;
                next = i;
            }
        }
        current = next;
        if (current >= 0) cur_pos = gs->enemies[current].pos;
    }
    audio_play(SFX_BEAM);
}

void chain_draw(const GameState *gs, float scale, Vector2 offset) {
    if (!gs->has_chain || gs->chain_visual.life <= 0) return;
    float alpha = gs->chain_visual.life / CHAIN_VISUAL_LIFE;
    Color col = {200, 240, 255, (unsigned char)(255 * alpha)};
    Color col_inner = {255, 255, 255, (unsigned char)(255 * alpha)};

    for (int i = 1; i < gs->chain_visual.count; i++) {
        Vector2 a = {
            gs->chain_visual.points[i - 1].x * scale + offset.x,
            gs->chain_visual.points[i - 1].y * scale + offset.y
        };
        Vector2 b = {
            gs->chain_visual.points[i].x * scale + offset.x,
            gs->chain_visual.points[i].y * scale + offset.y
        };
        DrawLineEx(a, b, 4.0f * scale, col);
        DrawLineEx(a, b, 1.5f * scale, col_inner);
    }
}

// === BOOMERANG ===

void boomerang_init(GameState *gs) {
    gs->has_boomerang = true;
    gs->boomerang_timer = BOOMERANG_INTERVAL;
    gs->boomerang_interval = BOOMERANG_INTERVAL;
    gs->boomerang_damage = BOOMERANG_DAMAGE;
    for (int i = 0; i < MAX_BOOMERANGS; i++) gs->boomerangs[i].active = false;
}

static void throw_boomerang(GameState *gs, Vector2 dir) {
    for (int i = 0; i < MAX_BOOMERANGS; i++) {
        if (!gs->boomerangs[i].active) {
            gs->boomerangs[i].active = true;
            gs->boomerangs[i].pos = gs->player.pos;
            gs->boomerangs[i].dir = dir;
            gs->boomerangs[i].traveled = 0;
            gs->boomerangs[i].returning = false;
            return;
        }
    }
}

void boomerang_update(GameState *gs, float dt) {
    if (!gs->has_boomerang) return;

    gs->boomerang_timer -= dt;
    if (gs->boomerang_timer <= 0) {
        gs->boomerang_timer = gs->boomerang_interval;
        Vector2 dir = {cosf(gs->player.facing_angle), sinf(gs->player.facing_angle)};
        throw_boomerang(gs, dir);
    }

    for (int i = 0; i < MAX_BOOMERANGS; i++) {
        if (!gs->boomerangs[i].active) continue;
        BoomerangProj *b = &gs->boomerangs[i];

        Vector2 move_dir;
        if (b->returning) {
            float dx = gs->player.pos.x - b->pos.x;
            float dy = gs->player.pos.y - b->pos.y;
            float len = sqrtf(dx * dx + dy * dy);
            if (len < BOOMERANG_RADIUS) {
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
                (BOOMERANG_RADIUS + gs->enemies[j].radius) *
                (BOOMERANG_RADIUS + gs->enemies[j].radius)) {
                damage_enemy_at(gs, j, gs->boomerang_damage);
            }
        }
        if (gs->boss.active) {
            float dx = b->pos.x - gs->boss.pos.x;
            float dy = b->pos.y - gs->boss.pos.y;
            if (dx * dx + dy * dy <
                (BOOMERANG_RADIUS + BOSS_RADIUS) *
                (BOOMERANG_RADIUS + BOSS_RADIUS)) {
                boss_take_damage(gs, gs->boomerang_damage);
            }
        }
    }
}

void boomerang_draw(const GameState *gs, float scale, Vector2 offset) {
    if (!gs->has_boomerang) return;
    float t = (float)GetTime();
    for (int i = 0; i < MAX_BOOMERANGS; i++) {
        if (!gs->boomerangs[i].active) continue;
        float x = gs->boomerangs[i].pos.x * scale + offset.x;
        float y = gs->boomerangs[i].pos.y * scale + offset.y;
        float r = BOOMERANG_RADIUS * scale;
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

// === TRAIL ===

void trail_init(GameState *gs) {
    gs->has_trail = true;
    gs->trail_timer = 0;
    gs->trail_damage = TRAIL_DAMAGE;
    for (int i = 0; i < MAX_TRAIL_MARKS; i++) gs->trail[i].active = false;
}

void trail_update(GameState *gs, float dt) {
    if (!gs->has_trail) return;

    gs->trail_timer -= dt;
    float vx = gs->player.vel.x;
    float vy = gs->player.vel.y;
    bool moving = (vx * vx + vy * vy) > 1.0f;
    if (gs->trail_timer <= 0 && moving) {
        gs->trail_timer = TRAIL_INTERVAL;
        for (int i = 0; i < MAX_TRAIL_MARKS; i++) {
            if (!gs->trail[i].active) {
                gs->trail[i].active = true;
                gs->trail[i].pos = gs->player.pos;
                gs->trail[i].life = TRAIL_LIFE;
                break;
            }
        }
    }

    for (int i = 0; i < MAX_TRAIL_MARKS; i++) {
        if (!gs->trail[i].active) continue;
        TrailMark *m = &gs->trail[i];
        m->life -= dt;
        if (m->life <= 0) {
            m->active = false;
            continue;
        }
        for (int j = 0; j < MAX_ENEMIES; j++) {
            if (!gs->enemies[j].active) continue;
            if (gs->enemies[j].phased) continue;
            float dx = gs->enemies[j].pos.x - m->pos.x;
            float dy = gs->enemies[j].pos.y - m->pos.y;
            if (dx * dx + dy * dy <
                (TRAIL_RADIUS + gs->enemies[j].radius) *
                (TRAIL_RADIUS + gs->enemies[j].radius)) {
                damage_enemy_at(gs, j, gs->trail_damage);
                m->life -= 0.3f;
            }
        }
    }
}

void trail_draw(const GameState *gs, float scale, Vector2 offset) {
    if (!gs->has_trail) return;
    for (int i = 0; i < MAX_TRAIL_MARKS; i++) {
        if (!gs->trail[i].active) continue;
        float ratio = gs->trail[i].life / TRAIL_LIFE;
        float x = gs->trail[i].pos.x * scale + offset.x;
        float y = gs->trail[i].pos.y * scale + offset.y;
        float r = TRAIL_RADIUS * scale * (0.6f + 0.4f * ratio);
        Color col = {120, 220, 255, (unsigned char)(180 * ratio)};
        Color inner = {200, 240, 255, (unsigned char)(220 * ratio)};
        DrawCircleV((Vector2){x, y}, r, col);
        DrawCircleV((Vector2){x, y}, r * 0.5f, inner);
    }
}

// === WHIP ===

void whip_init(GameState *gs) {
    gs->has_whip = true;
    gs->whip_timer = WHIP_INTERVAL;
    gs->whip_anim = 0;
    gs->whip_interval = WHIP_INTERVAL;
    gs->whip_damage = WHIP_DAMAGE;
}

void whip_update(GameState *gs, float dt) {
    if (!gs->has_whip) return;

    if (gs->whip_anim > 0) gs->whip_anim -= dt;

    gs->whip_timer -= dt;
    if (gs->whip_timer > 0) return;
    gs->whip_timer = gs->whip_interval;
    gs->whip_anim = WHIP_ANIM;

    float center = gs->player.facing_angle;
    float r2 = WHIP_RANGE * WHIP_RANGE;

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!gs->enemies[i].active) continue;
        if (gs->enemies[i].phased) continue;
        float dx = gs->enemies[i].pos.x - gs->player.pos.x;
        float dy = gs->enemies[i].pos.y - gs->player.pos.y;
        float d2 = dx * dx + dy * dy;
        if (d2 > r2) continue;
        float ang = atan2f(dy, dx);
        float diff = ang - center;
        while (diff > 3.14159f) diff -= 2.0f * 3.14159f;
        while (diff < -3.14159f) diff += 2.0f * 3.14159f;
        if (fabsf(diff) < WHIP_ARC * 0.5f) {
            damage_enemy_at(gs, i, gs->whip_damage);
        }
    }
    if (gs->boss.active) {
        float dx = gs->boss.pos.x - gs->player.pos.x;
        float dy = gs->boss.pos.y - gs->player.pos.y;
        float d2 = dx * dx + dy * dy;
        if (d2 < r2) {
            float ang = atan2f(dy, dx);
            float diff = ang - center;
            while (diff > 3.14159f) diff -= 2.0f * 3.14159f;
            while (diff < -3.14159f) diff += 2.0f * 3.14159f;
            if (fabsf(diff) < WHIP_ARC * 0.5f) {
                boss_take_damage(gs, gs->whip_damage);
            }
        }
    }
    audio_play(SFX_SHOOT);
}

void whip_draw(const GameState *gs, float scale, Vector2 offset) {
    if (!gs->has_whip || gs->whip_anim <= 0) return;

    float px = gs->player.pos.x * scale + offset.x;
    float py = gs->player.pos.y * scale + offset.y;
    float r = WHIP_RANGE * scale;
    float alpha = gs->whip_anim / WHIP_ANIM;
    float center = gs->player.facing_angle * (180.0f / 3.14159f);
    float arc_deg = WHIP_ARC * (180.0f / 3.14159f);

    Color outer = {220, 220, 255, (unsigned char)(180 * alpha)};
    Color inner = {255, 255, 255, (unsigned char)(220 * alpha)};

    DrawRing((Vector2){px, py}, r * 0.8f, r,
        center - arc_deg * 0.5f, center + arc_deg * 0.5f, 24, outer);
    DrawRing((Vector2){px, py}, r * 0.9f, r * 0.95f,
        center - arc_deg * 0.5f, center + arc_deg * 0.5f, 24, inner);
}
