#include "game.h"
#include <math.h>
#include <stdlib.h>

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

    if (b->hp <= 0) {
        b->active = false;
        gs->boss_defeated = true;
        gs->kills += 10;

        particles_spawn_burst(gs, b->pos, (Color){255, 200, 50, 255}, 60);
        shake_add(gs, SHAKE_BOSS_HIT * 2);

        for (int i = 0; i < 20; i++) {
            float angle = (2.0f * 3.14159f * i) / 20.0f;
            Vector2 pos = {
                b->pos.x + cosf(angle) * 30,
                b->pos.y + sinf(angle) * 30
            };
            gem_spawn(gs, pos);
        }
    }
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
}
