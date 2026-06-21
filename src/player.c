#include "game.h"
#include <math.h>

void player_init(Player *p, float scale) {
    (void)scale;
    p->pos.x = LOGICAL_W / 2.0f;
    p->pos.y = LOGICAL_H / 2.0f;
    p->vel.x = 0;
    p->vel.y = 0;
    p->facing_angle = 0;
    int hp_scaled = (int)(PLAYER_MAX_HP * g_params.player_hp_mult + 0.5f);
    if (hp_scaled < 1) hp_scaled = 1;
    p->hp = hp_scaled;
    p->max_hp = hp_scaled;
    p->invincible_timer = 0;
    p->speed = PLAYER_SPEED * g_params.player_speed_mult;
    p->pickup_range = GEM_PICKUP_RANGE;
}

void player_update(GameState *gs, float dt, float scale) {
    Player *p = &gs->player;
    if (p->invincible_timer > 0) {
        p->invincible_timer -= dt;
    }

    Vector2 dir;
    if (gs->bot_mode) {
        dir = bot_compute_direction(gs);
    } else {
        dir = input_get_move_direction();
        if (input_is_mouse_active()) {
            Vector2 screen_pos = {p->pos.x * scale, p->pos.y * scale};
            dir = input_get_mouse_direction(screen_pos);
        }
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

bool player_take_damage(GameState *gs, int damage) {
    Player *p = &gs->player;
    if (gs->debug_invincible) return false;
    if (p->invincible_timer > 0) return false;
    int scaled = (int)(damage * g_params.enemy_damage_mult + 0.5f);
    if (scaled < 1) scaled = 1;
    p->hp -= scaled;
    if (p->hp < 0) p->hp = 0;
    gs->stats.damage_taken += scaled;
    p->invincible_timer = PLAYER_INVINCIBLE_TIME * g_params.player_invincible_mult;
    audio_play(SFX_PLAYER_HIT);
    particles_spawn_burst(gs, p->pos, (Color){255, 255, 255, 255}, 16);
    shake_add(gs, SHAKE_HIT);
    return true;
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

    // Outline ring: dark halo (separation from bright effects) + cyan glow ring
    // (pulsing to keep "me" salient even in chaos). Drawn UNDER the body so the
    // body color still pops; ring extends well past the hit radius.
    float t = (float)GetTime();
    float ring_pulse = 1.0f + 0.08f * sinf(t * 4.5f);
    float ring_r = r * 1.30f * ring_pulse;

    // Dark separator (notch the player out of any additive blowout behind it)
    DrawCircleV((Vector2){x, y}, ring_r + 1.5f, (Color){0, 0, 0, 200});
    // Cyan glow ring
    DrawCircleV((Vector2){x, y}, ring_r, (Color){80, 220, 255, 110});
    DrawCircleV((Vector2){x, y}, ring_r * 0.88f, (Color){0, 0, 0, 230});

    // Body
    DrawCircleV((Vector2){x, y}, r, (Color){0, 255, 255, 255});
    DrawCircleV((Vector2){x, y}, r * 0.6f, (Color){200, 255, 255, 230});
    // White hot core: highest luminance pixel on screen, marks "you are here"
    DrawCircleV((Vector2){x, y}, r * 0.28f, (Color){255, 255, 255, 255});
}

void player_draw_hp_bar(const Player *p, float scale, Vector2 offset) {
    float r = PLAYER_RADIUS * scale;
    float x = p->pos.x * scale + offset.x;
    float y = p->pos.y * scale + offset.y;

    int bar_w = (int)(r * 3.0f);
    int bar_h = (int)(3.0f * scale);
    if (bar_h < 3) bar_h = 3;
    int bar_x = (int)(x - bar_w / 2);
    int bar_y = (int)(y - r - 10 * scale);

    float ratio = (float)p->hp / p->max_hp;
    if (ratio < 0) ratio = 0;

    Color fg;
    if (ratio > 0.6f)      fg = (Color){100, 255, 150, 255};
    else if (ratio > 0.3f) fg = (Color){255, 220, 80, 255};
    else                   fg = (Color){255, 80, 80, 255};

    DrawRectangle(bar_x - 1, bar_y - 1, bar_w + 2, bar_h + 2, (Color){0, 0, 0, 180});
    DrawRectangle(bar_x, bar_y, bar_w, bar_h, (Color){40, 40, 60, 220});
    DrawRectangle(bar_x, bar_y, (int)(bar_w * ratio), bar_h, fg);
}
