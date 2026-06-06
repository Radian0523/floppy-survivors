#include "game.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

void boss_spawn(GameState *gs) {
    Boss *b = &gs->boss;
    b->active = true;
    b->hp = BOSS_HP;
    b->max_hp = BOSS_HP;
    audio_play(SFX_BOSS_SPAWN);

    int side = rand() % 4;
    switch (side) {
        case 0:
            b->pos.x = LOGICAL_W / 2;
            b->pos.y = -BOSS_RADIUS * 2;
            break;
        case 1:
            b->pos.x = LOGICAL_W / 2;
            b->pos.y = LOGICAL_H + BOSS_RADIUS * 2;
            break;
        case 2:
            b->pos.x = -BOSS_RADIUS * 2;
            b->pos.y = LOGICAL_H / 2;
            break;
        default:
            b->pos.x = LOGICAL_W + BOSS_RADIUS * 2;
            b->pos.y = LOGICAL_H / 2;
            break;
    }

    b->vel.x = 0;
    b->vel.y = 0;
    b->charging = false;
    b->charge_timer = 0;
    b->charge_cooldown = 2.0f;
    b->spawn_timer = BOSS_SPAWN_INTERVAL;
}

void boss_update(GameState *gs, float dt) {
    Boss *b = &gs->boss;
    if (!b->active) return;

    float dx = gs->player.pos.x - b->pos.x;
    float dy = gs->player.pos.y - b->pos.y;
    float dist = sqrtf(dx * dx + dy * dy);

    if (b->charging) {
        b->pos.x += b->charge_dir.x * BOSS_CHARGE_SPEED * dt;
        b->pos.y += b->charge_dir.y * BOSS_CHARGE_SPEED * dt;

        b->charge_timer -= dt;
        if (b->charge_timer <= 0) {
            b->charging = false;
            b->charge_cooldown = BOSS_CHARGE_COOLDOWN;
        }
    } else {
        if (dist > 0) {
            b->vel.x = (dx / dist) * BOSS_SPEED;
            b->vel.y = (dy / dist) * BOSS_SPEED;
        }
        b->pos.x += b->vel.x * dt;
        b->pos.y += b->vel.y * dt;

        b->charge_cooldown -= dt;
        if (b->charge_cooldown <= 0 && dist > 50) {
            b->charging = true;
            b->charge_timer = BOSS_CHARGE_DURATION;
            b->charge_dir.x = dx / dist;
            b->charge_dir.y = dy / dist;
        }
    }

    b->spawn_timer -= dt;
    if (b->spawn_timer <= 0) {
        b->spawn_timer = BOSS_SPAWN_INTERVAL;
        for (int i = 0; i < 3; i++) {
            float angle = (2.0f * 3.14159f * i) / 3.0f;
            Vector2 spawn_pos = {
                b->pos.x + cosf(angle) * (BOSS_RADIUS + 20),
                b->pos.y + sinf(angle) * (BOSS_RADIUS + 20)
            };
            enemy_spawn_at(gs, ENEMY_FRAGMENT, spawn_pos);
        }
    }

    if (dist < PLAYER_RADIUS + BOSS_RADIUS) {
        player_take_damage(gs, BOSS_DAMAGE);
    }
}

void boss_take_damage(GameState *gs, int damage) {
    Boss *b = &gs->boss;
    if (!b->active) return;

    b->hp -= damage;
    audio_play(SFX_BOSS_HIT);
    shake_add(gs, 1.0f);
    popup_spawn(gs, b->pos, damage, (Color){255, 255, 100, 255});

    if (b->hp <= 0) {
        b->active = false;
        gs->boss_defeated = true;
        gs->kills += 10;

        particles_spawn_burst(gs, b->pos, (Color){255, 200, 50, 255}, 60);
        shake_add(gs, SHAKE_BOSS_HIT * 2);

        for (int i = 0; i < 8; i++) {
            float angle = (2.0f * 3.14159f * i) / 8.0f;
            Vector2 pos = {
                b->pos.x + cosf(angle) * 30,
                b->pos.y + sinf(angle) * 30
            };
            gem_spawn_tier(gs, pos, GEM_TIER_L);
        }
        for (int i = 0; i < 12; i++) {
            float angle = (2.0f * 3.14159f * i) / 12.0f + 0.2f;
            Vector2 pos = {
                b->pos.x + cosf(angle) * 50,
                b->pos.y + sinf(angle) * 50
            };
            gem_spawn_tier(gs, pos, GEM_TIER_M);
        }
    }
}

static void draw_boss_indicator(const GameState *gs, float scale, Vector2 offset) {
    const Boss *b = &gs->boss;

    float margin = BOSS_RADIUS * 0.5f;
    bool off_left = b->pos.x < margin;
    bool off_right = b->pos.x > LOGICAL_W - margin;
    bool off_top = b->pos.y < margin;
    bool off_bottom = b->pos.y > LOGICAL_H - margin;

    if (!off_left && !off_right && !off_top && !off_bottom) return;

    float bx = b->pos.x;
    float by = b->pos.y;
    float edge_margin = 30.0f;
    if (bx < edge_margin) bx = edge_margin;
    if (bx > LOGICAL_W - edge_margin) bx = LOGICAL_W - edge_margin;
    if (by < edge_margin) by = edge_margin;
    if (by > LOGICAL_H - edge_margin) by = LOGICAL_H - edge_margin;

    float dx = b->pos.x - bx;
    float dy = b->pos.y - by;
    float len = sqrtf(dx * dx + dy * dy);
    float angle = (len > 0) ? atan2f(dy, dx) : 0;

    float sx = bx * scale + offset.x;
    float sy = by * scale + offset.y;
    float arrow_size = 18.0f * scale;

    float pulse = 0.7f + 0.3f * sinf((float)GetTime() * 6.0f);
    Color col = {255, 200, 50, (unsigned char)(255 * pulse)};
    Color glow = {255, 100, 50, (unsigned char)(120 * pulse)};

    Vector2 tip = {sx + cosf(angle) * arrow_size, sy + sinf(angle) * arrow_size};
    Vector2 base_l = {
        sx + cosf(angle + 2.4f) * arrow_size * 0.7f,
        sy + sinf(angle + 2.4f) * arrow_size * 0.7f
    };
    Vector2 base_r = {
        sx + cosf(angle - 2.4f) * arrow_size * 0.7f,
        sy + sinf(angle - 2.4f) * arrow_size * 0.7f
    };

    DrawCircleV((Vector2){sx, sy}, arrow_size * 1.2f, glow);
    DrawTriangle(tip, base_l, base_r, col);
    DrawTriangle(tip, base_r, base_l, col);

    char hp_buf[16];
    sprintf(hp_buf, "%d", b->hp);
    int tw = MeasureText(hp_buf, 12);
    DrawText(hp_buf, (int)(sx - tw / 2), (int)(sy + arrow_size + 2), 12,
        (Color){255, 200, 100, 255});
}

void boss_draw(const GameState *gs, float scale, Vector2 offset) {
    const Boss *b = &gs->boss;
    if (!b->active) return;

    float x = b->pos.x * scale + offset.x;
    float y = b->pos.y * scale + offset.y;
    float r = BOSS_RADIUS * scale;

    Color outer = b->charging ? (Color){255, 50, 50, 255} : (Color){255, 200, 50, 255};
    Color inner = b->charging ? (Color){255, 150, 150, 255} : (Color){255, 255, 150, 255};

    DrawCircleV((Vector2){x, y}, r * 1.2f, (Color){outer.r, outer.g, outer.b, 80});
    DrawCircleV((Vector2){x, y}, r, outer);
    DrawCircleV((Vector2){x, y}, r * 0.7f, inner);
    DrawCircleV((Vector2){x, y}, r * 0.4f, outer);

    DrawRing((Vector2){x, y}, r * 0.9f, r * 1.1f, 0, 360, 6, (Color){255, 255, 255, 150});

    float hp_ratio = (float)b->hp / b->max_hp;
    int bar_w = (int)(r * 2);
    int bar_h = 6;
    int bar_x = (int)(x - r);
    int bar_y = (int)(y - r - 15);
    DrawRectangle(bar_x, bar_y, bar_w, bar_h, (Color){50, 50, 50, 200});
    DrawRectangle(bar_x, bar_y, (int)(bar_w * hp_ratio), bar_h, (Color){255, 100, 100, 255});

    const char *name = "FORMAT";
    int tw = MeasureText(name, 14);
    DrawText(name, (int)(x - tw / 2), bar_y - 18, 14, (Color){255, 200, 100, 255});

    draw_boss_indicator(gs, scale, offset);
}
