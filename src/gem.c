#include "game.h"
#include <math.h>

void gem_spawn(GameState *gs, Vector2 pos) {
    for (int i = 0; i < MAX_GEMS; i++) {
        if (!gs->gems[i].active) {
            gs->gems[i].active = true;
            gs->gems[i].pos = pos;
            return;
        }
    }
}

void gem_update(GameState *gs, float dt) {
    for (int i = 0; i < MAX_GEMS; i++) {
        if (!gs->gems[i].active) continue;

        Gem *g = &gs->gems[i];
        float dx = gs->player.pos.x - g->pos.x;
        float dy = gs->player.pos.y - g->pos.y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist < gs->player.pickup_range) {
            float pull_speed = 200.0f;
            if (dist > 0) {
                g->pos.x += (dx / dist) * pull_speed * dt;
                g->pos.y += (dy / dist) * pull_speed * dt;
            }
        }

        if (dist < PLAYER_RADIUS + GEM_RADIUS) {
            Vector2 pickup_pos = g->pos;
            g->active = false;
            gs->xp += GEM_XP_VALUE;
            audio_play(SFX_GEM_PICKUP);
            particles_spawn_burst(gs, pickup_pos, (Color){150, 255, 150, 255}, 4);

            if (gs->xp >= gs->xp_to_next) {
                gs->xp -= gs->xp_to_next;
                gs->level++;
                gs->xp_to_next += XP_PER_LEVEL;
                upgrade_start(gs);
                audio_play(SFX_LEVEL_UP);
                flash_trigger(gs, (Color){255, 255, 200, 255}, 0.6f);
                particles_spawn_burst(gs, gs->player.pos,
                    (Color){255, 255, 150, 255}, 30);
            }
        }
    }
}

void gem_draw(const Gem gems[], float scale, Vector2 offset) {
    float t = (float)GetTime();
    for (int i = 0; i < MAX_GEMS; i++) {
        if (!gems[i].active) continue;
        float x = gems[i].pos.x * scale + offset.x;
        float y = gems[i].pos.y * scale + offset.y;
        float r = GEM_RADIUS * scale;
        float pulse = 1.0f + 0.15f * sinf(t * 4.0f + i * 0.3f);
        float rr = r * pulse;

        Color outer = {120, 255, 120, 255};
        Color inner = {220, 255, 220, 255};

        Vector2 top    = {x, y - rr * 1.2f};
        Vector2 right  = {x + rr * 0.9f, y};
        Vector2 bottom = {x, y + rr * 1.2f};
        Vector2 left   = {x - rr * 0.9f, y};

        DrawTriangle(top, left, right, outer);
        DrawTriangle(bottom, right, left, outer);

        Vector2 itop    = {x, y - rr * 0.5f};
        Vector2 iright  = {x + rr * 0.4f, y};
        Vector2 ibottom = {x, y + rr * 0.5f};
        Vector2 ileft   = {x - rr * 0.4f, y};
        DrawTriangle(itop, ileft, iright, inner);
        DrawTriangle(ibottom, iright, ileft, inner);
    }
}
