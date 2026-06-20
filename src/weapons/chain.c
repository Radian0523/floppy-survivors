#include "../game.h"
#include "../weapon_util.h"
#include <math.h>

void chain_init(GameState *gs) {
    gs->chain.has = true;
    gs->chain.timer = CHAIN_INTERVAL;
    gs->chain.interval = CHAIN_INTERVAL;
    gs->chain.jumps = CHAIN_JUMPS;
    gs->chain.damage = CHAIN_DAMAGE;
    gs->chain.visual.count = 0;
    gs->chain.visual.life = 0;
}

void chain_update(GameState *gs, float dt) {
    if (!gs->chain.has) return;

    if (gs->chain.visual.life > 0) gs->chain.visual.life -= dt;

    gs->chain.timer -= dt;
    if (gs->chain.timer > 0) return;
    gs->chain.timer = gs->chain.interval * gs->weapon_rate_mult;

    int first = weapon_nearest_enemy(gs, gs->player.pos, CHAIN_RANGE);
    if (first < 0) return;

    gs->chain.visual.count = 0;
    gs->chain.visual.life = CHAIN_VISUAL_LIFE * gs->weapon_duration_mult;
    gs->chain.visual.points[gs->chain.visual.count++] = gs->player.pos;

    bool hit[MAX_ENEMIES] = {false};
    int current = first;
    Vector2 cur_pos = gs->enemies[current].pos;

    for (int j = 0; j < gs->chain.jumps && current >= 0; j++) {
        hit[current] = true;
        gs->chain.visual.points[gs->chain.visual.count++] = cur_pos;
        weapon_hit_enemy(gs, current, gs->chain.damage, (Color){200, 240, 255, 255}, WEAPON_ID_CHAIN);

        if (gs->chain.visual.count >= CHAIN_MAX_POINTS) break;

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
    if (!gs->chain.has || gs->chain.visual.life <= 0) return;
    float life_max = CHAIN_VISUAL_LIFE * gs->weapon_duration_mult;
    float alpha = (life_max > 0) ? gs->chain.visual.life / life_max : 0;
    Color col = {200, 240, 255, (unsigned char)(255 * alpha)};
    Color col_inner = {255, 255, 255, (unsigned char)(255 * alpha)};

    for (int i = 1; i < gs->chain.visual.count; i++) {
        Vector2 a = {
            gs->chain.visual.points[i - 1].x * scale + offset.x,
            gs->chain.visual.points[i - 1].y * scale + offset.y
        };
        Vector2 b = {
            gs->chain.visual.points[i].x * scale + offset.x,
            gs->chain.visual.points[i].y * scale + offset.y
        };
        DrawLineEx(a, b, 4.0f * scale, col);
        DrawLineEx(a, b, 1.5f * scale, col_inner);
    }
}
