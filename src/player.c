#include "game.h"
#include <math.h>

void player_init(Player *p, float scale) {
    (void)scale;
    p->pos.x = LOGICAL_W / 2.0f;
    p->pos.y = LOGICAL_H / 2.0f;
    p->vel.x = 0;
    p->vel.y = 0;
    p->facing_angle = 0;
    p->hp = PLAYER_MAX_HP;
    p->max_hp = PLAYER_MAX_HP;
    p->invincible_timer = 0;
    p->speed = PLAYER_SPEED;
    p->pickup_range = GEM_PICKUP_RANGE;
}

void player_update(Player *p, float dt, float scale) {
    if (p->invincible_timer > 0) {
        p->invincible_timer -= dt;
    }

    Vector2 dir = input_get_move_direction();

    if (input_is_mouse_active()) {
        Vector2 screen_pos = {p->pos.x * scale, p->pos.y * scale};
        dir = input_get_mouse_direction(screen_pos);
    }

    p->vel.x = dir.x * p->speed;
    p->vel.y = dir.y * p->speed;

    if (dir.x != 0 || dir.y != 0) {
        p->facing_angle = atan2f(dir.y, dir.x);
    }

    p->pos.x += p->vel.x * dt;
    p->pos.y += p->vel.y * dt;

    float margin = PLAYER_RADIUS;
    if (p->pos.x < margin) p->pos.x = margin;
    if (p->pos.x > LOGICAL_W - margin) p->pos.x = LOGICAL_W - margin;
    if (p->pos.y < margin) p->pos.y = margin;
    if (p->pos.y > LOGICAL_H - margin) p->pos.y = LOGICAL_H - margin;
}

void player_take_damage(GameState *gs, int damage) {
    Player *p = &gs->player;
    if (p->invincible_timer > 0) return;
    p->hp -= damage;
    if (p->hp < 0) p->hp = 0;
    p->invincible_timer = PLAYER_INVINCIBLE_TIME;
    audio_play(SFX_PLAYER_HIT);
    particles_spawn_burst(gs, p->pos, (Color){255, 255, 255, 255}, 16);
    shake_add(gs, SHAKE_HIT);
}

void player_draw(const Player *p, float scale, Vector2 offset) {
    float r = PLAYER_RADIUS * scale;
    float x = p->pos.x * scale + offset.x;
    float y = p->pos.y * scale + offset.y;

    if (p->invincible_timer > 0) {
        int blink = (int)(p->invincible_timer * 15) % 2;
        if (blink) {
            float pulse = 1.0f + 0.3f * sinf(p->invincible_timer * 30);
            DrawCircleV((Vector2){x, y}, r * pulse * 1.3f, (Color){255, 100, 100, 100});
            DrawCircleV((Vector2){x, y}, r * pulse, (Color){255, 255, 255, 200});
            DrawCircleV((Vector2){x, y}, r * pulse * 0.5f, (Color){255, 200, 200, 255});
        }
        return;
    }

    DrawCircleV((Vector2){x, y}, r, (Color){0, 255, 255, 255});
    DrawCircleV((Vector2){x, y}, r * 0.6f, (Color){100, 255, 255, 200});
}
