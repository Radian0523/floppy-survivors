#include "game.h"
#include <math.h>
#include <float.h>

// Heuristic bot AI:
// - Strong "flee from enemies" force (weighted by 1/distance^2)
// - Pull toward nearest gem/item if not under threat
// - Steer away from screen edges

#define BOT_THREAT_RANGE 140.0f
#define BOT_GEM_RANGE    220.0f
#define BOT_EDGE_MARGIN  40.0f

static void add_force(float *fx, float *fy, float x, float y, float w) {
    *fx += x * w;
    *fy += y * w;
}

Vector2 bot_compute_direction(const GameState *gs) {
    float fx = 0, fy = 0;
    Vector2 pp = gs->player.pos;

    // 1) Flee from enemies
    float threat_strength = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!gs->enemies[i].active) continue;
        float dx = pp.x - gs->enemies[i].pos.x;
        float dy = pp.y - gs->enemies[i].pos.y;
        float d = sqrtf(dx * dx + dy * dy);
        if (d <= 0.01f) continue;
        if (d > BOT_THREAT_RANGE) continue;
        float w = (BOT_THREAT_RANGE - d) / BOT_THREAT_RANGE;
        w = w * w;
        add_force(&fx, &fy, dx / d, dy / d, w * 5.0f);
        if (w > threat_strength) threat_strength = w;
    }

    // Boss flee (large radius)
    if (gs->boss.active) {
        float dx = pp.x - gs->boss.pos.x;
        float dy = pp.y - gs->boss.pos.y;
        float d = sqrtf(dx * dx + dy * dy);
        if (d > 0.01f) {
            float reach = 160.0f;
            if (d < reach) {
                float w = (reach - d) / reach;
                add_force(&fx, &fy, dx / d, dy / d, w * 6.0f);
                if (w > threat_strength) threat_strength = w;
            }
        }
    }

    // Enemy bullets
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (!gs->enemy_bullets[i].active) continue;
        float dx = pp.x - gs->enemy_bullets[i].pos.x;
        float dy = pp.y - gs->enemy_bullets[i].pos.y;
        float d = sqrtf(dx * dx + dy * dy);
        if (d > 0.01f && d < 80.0f) {
            float w = (80.0f - d) / 80.0f;
            add_force(&fx, &fy, dx / d, dy / d, w * 4.0f);
            if (w > threat_strength) threat_strength = w;
        }
    }

    // 2) Approach nearest gem (only if not under heavy threat)
    if (threat_strength < 0.5f) {
        int best = -1;
        float bd2 = FLT_MAX;
        for (int i = 0; i < MAX_GEMS; i++) {
            if (!gs->gems[i].active) continue;
            float dx = gs->gems[i].pos.x - pp.x;
            float dy = gs->gems[i].pos.y - pp.y;
            float d2 = dx * dx + dy * dy;
            if (d2 < bd2) {
                bd2 = d2;
                best = i;
            }
        }
        if (best >= 0) {
            float dx = gs->gems[best].pos.x - pp.x;
            float dy = gs->gems[best].pos.y - pp.y;
            float d = sqrtf(dx * dx + dy * dy);
            if (d > 0.01f && d < BOT_GEM_RANGE) {
                add_force(&fx, &fy, dx / d, dy / d, 1.5f * (1.0f - threat_strength));
            }
        }

        // Items / chests are higher priority
        for (int i = 0; i < MAX_ITEMS; i++) {
            if (!gs->items[i].active) continue;
            float dx = gs->items[i].pos.x - pp.x;
            float dy = gs->items[i].pos.y - pp.y;
            float d = sqrtf(dx * dx + dy * dy);
            if (d > 0.01f && d < BOT_GEM_RANGE * 1.5f) {
                add_force(&fx, &fy, dx / d, dy / d, 3.0f);
            }
        }
        for (int i = 0; i < MAX_CHESTS; i++) {
            if (!gs->chests[i].active) continue;
            float dx = gs->chests[i].pos.x - pp.x;
            float dy = gs->chests[i].pos.y - pp.y;
            float d = sqrtf(dx * dx + dy * dy);
            if (d > 0.01f && d < BOT_GEM_RANGE * 1.5f) {
                add_force(&fx, &fy, dx / d, dy / d, 4.0f);
            }
        }
    }

    // 3) Avoid screen edges (logical coords)
    if (pp.x < BOT_EDGE_MARGIN)
        add_force(&fx, &fy, 1.0f, 0, (BOT_EDGE_MARGIN - pp.x) / BOT_EDGE_MARGIN * 3.0f);
    if (pp.x > LOGICAL_W - BOT_EDGE_MARGIN)
        add_force(&fx, &fy, -1.0f, 0,
            (pp.x - (LOGICAL_W - BOT_EDGE_MARGIN)) / BOT_EDGE_MARGIN * 3.0f);
    if (pp.y < BOT_EDGE_MARGIN)
        add_force(&fx, &fy, 0, 1.0f, (BOT_EDGE_MARGIN - pp.y) / BOT_EDGE_MARGIN * 3.0f);
    if (pp.y > LOGICAL_H - BOT_EDGE_MARGIN)
        add_force(&fx, &fy, 0, -1.0f,
            (pp.y - (LOGICAL_H - BOT_EDGE_MARGIN)) / BOT_EDGE_MARGIN * 3.0f);

    float len = sqrtf(fx * fx + fy * fy);
    Vector2 out = {0, 0};
    if (len > 0.01f) {
        out.x = fx / len;
        out.y = fy / len;
    }
    return out;
}
