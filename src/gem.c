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
            g->active = false;
            gs->xp += GEM_XP_VALUE;

            if (gs->xp >= gs->xp_to_next) {
                gs->xp -= gs->xp_to_next;
                gs->level++;
                gs->xp_to_next += XP_PER_LEVEL;
            }
        }
    }
}

void gem_draw(const Gem gems[], float scale, Vector2 offset) {
    for (int i = 0; i < MAX_GEMS; i++) {
        if (!gems[i].active) continue;
        float x = gems[i].pos.x * scale + offset.x;
        float y = gems[i].pos.y * scale + offset.y;
        float r = GEM_RADIUS * scale;
        DrawCircleV((Vector2){x, y}, r, (Color){100, 255, 100, 255});
        DrawCircleV((Vector2){x, y}, r * 0.5f, (Color){150, 255, 150, 200});
    }
}
