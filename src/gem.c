#include "game.h"
#include <math.h>

void gem_spawn_tier(GameState *gs, Vector2 pos, GemTier tier) {
    float margin = GEM_RADIUS + 2.0f;
    if (pos.x < margin) pos.x = margin;
    if (pos.x > LOGICAL_W - margin) pos.x = LOGICAL_W - margin;
    if (pos.y < margin) pos.y = margin;
    if (pos.y > LOGICAL_H - margin) pos.y = LOGICAL_H - margin;

    for (int i = 0; i < MAX_GEMS; i++) {
        if (!gs->gems[i].active) {
            gs->gems[i].active = true;
            gs->gems[i].pos = pos;
            gs->gems[i].tier = tier;
            return;
        }
    }
}

void gem_spawn(GameState *gs, Vector2 pos) {
    gem_spawn_tier(gs, pos, GEM_TIER_S);
}

static int gem_xp_value(GemTier tier) {
    switch (tier) {
        case GEM_TIER_S: return GEM_XP_S;
        case GEM_TIER_M: return GEM_XP_M;
        case GEM_TIER_L: return GEM_XP_L;
        default: return GEM_XP_S;
    }
}

void gem_update(GameState *gs, float dt) {
    if (gs->magnet_pull_timer > 0) gs->magnet_pull_timer -= dt;
    bool magnet_active = gs->magnet_pull_timer > 0;

    for (int i = 0; i < MAX_GEMS; i++) {
        if (!gs->gems[i].active) continue;

        Gem *g = &gs->gems[i];
        float dx = gs->player.pos.x - g->pos.x;
        float dy = gs->player.pos.y - g->pos.y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (magnet_active) {
            float pull_speed = 600.0f;
            if (dist > 0) {
                g->pos.x += (dx / dist) * pull_speed * dt;
                g->pos.y += (dy / dist) * pull_speed * dt;
            }
        } else if (dist < gs->player.pickup_range) {
            float pull_speed = 200.0f;
            if (dist > 0) {
                g->pos.x += (dx / dist) * pull_speed * dt;
                g->pos.y += (dy / dist) * pull_speed * dt;
            }
        }

        if (dist < PLAYER_RADIUS + GEM_RADIUS) {
            Vector2 pickup_pos = g->pos;
            int xp_gained = gem_xp_value(g->tier);
            g->active = false;
            gs->xp += xp_gained;
            audio_play(SFX_GEM_PICKUP);
            Color spark = (g->tier == GEM_TIER_L) ?
                (Color){180, 150, 255, 255} :
                (g->tier == GEM_TIER_M) ?
                (Color){150, 240, 255, 255} :
                (Color){150, 255, 150, 255};
            particles_spawn_burst(gs, pickup_pos, spark,
                                   3 + (int)g->tier * 2);

            while (gs->xp >= gs->xp_to_next) {
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

typedef struct {
    Color outer;
    Color inner;
    float size_mult;
} GemStyle;

static GemStyle gem_style(GemTier tier) {
    GemStyle s = {0};
    s.size_mult = 1.0f;
    switch (tier) {
        case GEM_TIER_S:
            s.outer = (Color){120, 255, 120, 255};
            s.inner = (Color){220, 255, 220, 255};
            break;
        case GEM_TIER_M:
            s.outer = (Color){100, 220, 255, 255};
            s.inner = (Color){200, 240, 255, 255};
            break;
        case GEM_TIER_L:
            s.outer = (Color){180, 130, 255, 255};
            s.inner = (Color){230, 200, 255, 255};
            break;
        default:
            s.outer = (Color){120, 255, 120, 255};
            s.inner = (Color){220, 255, 220, 255};
            break;
    }
    return s;
}

void gem_draw(const Gem gems[], float scale, Vector2 offset) {
    float t = (float)GetTime();
    for (int i = 0; i < MAX_GEMS; i++) {
        if (!gems[i].active) continue;
        float x = gems[i].pos.x * scale + offset.x;
        float y = gems[i].pos.y * scale + offset.y;
        GemStyle st = gem_style(gems[i].tier);
        float r = GEM_RADIUS * scale * st.size_mult;
        float pulse = 1.0f + 0.15f * sinf(t * 4.0f + i * 0.3f);
        float rr = r * pulse;

        // Outer glow for higher tiers
        if (gems[i].tier == GEM_TIER_L) {
            DrawCircleV((Vector2){x, y}, rr * 1.6f,
                (Color){st.outer.r, st.outer.g, st.outer.b, 60});
        } else if (gems[i].tier == GEM_TIER_M) {
            DrawCircleV((Vector2){x, y}, rr * 1.4f,
                (Color){st.outer.r, st.outer.g, st.outer.b, 40});
        }

        Vector2 top    = {x, y - rr * 1.2f};
        Vector2 right  = {x + rr * 0.9f, y};
        Vector2 bottom = {x, y + rr * 1.2f};
        Vector2 left   = {x - rr * 0.9f, y};

        DrawTriangle(top, left, right, st.outer);
        DrawTriangle(bottom, right, left, st.outer);

        Vector2 itop    = {x, y - rr * 0.5f};
        Vector2 iright  = {x + rr * 0.4f, y};
        Vector2 ibottom = {x, y + rr * 0.5f};
        Vector2 ileft   = {x - rr * 0.4f, y};
        DrawTriangle(itop, ileft, iright, st.inner);
        DrawTriangle(ibottom, iright, ileft, st.inner);

        // Sparkle on the largest gems
        if (gems[i].tier == GEM_TIER_L) {
            float a = t * 2.0f + i;
            DrawLineEx((Vector2){x - rr * 1.1f, y}, (Vector2){x + rr * 1.1f, y},
                       1.5f * scale,
                       (Color){255, 255, 255, (unsigned char)(60 + 60 * sinf(a))});
            DrawLineEx((Vector2){x, y - rr * 1.3f}, (Vector2){x, y + rr * 1.3f},
                       1.0f * scale,
                       (Color){255, 255, 255, (unsigned char)(40 + 40 * sinf(a + 1.5f))});
        }
    }
}
