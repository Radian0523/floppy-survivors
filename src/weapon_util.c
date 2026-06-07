#include "weapon_util.h"
#include <math.h>
#include <float.h>
#include <stdlib.h>

Color enemy_color_for_type(EnemyType type) {
    switch (type) {
        case ENEMY_BIT:       return (Color){255, 50, 100, 255};
        case ENEMY_FRAGMENT:  return (Color){255, 180, 60, 255};
        case ENEMY_PACKET:    return (Color){255, 120, 40, 255};
        case ENEMY_GLITCH:    return (Color){220, 80, 180, 255};
        case ENEMY_SPLITTER:  return (Color){255, 80, 60, 255};
        case ENEMY_BOMBER:    return (Color){255, 60, 140, 255};
        case ENEMY_RANGER:    return (Color){255, 100, 180, 255};
        case ENEMY_SWARM:     return (Color){255, 200, 180, 255};
        case ENEMY_BADSECTOR: return (Color){200, 60, 60, 255};
        case ENEMY_PHASER:    return (Color){230, 100, 100, 255};
        case ENEMY_TRACKER:   return (Color){255, 90, 70, 255};
        default:              return (Color){255, 200, 100, 255};
    }
}

static GemTier gem_tier_for_enemy(EnemyType type, bool elite) {
    if (elite) return GEM_TIER_L;
    switch (type) {
        case ENEMY_BIT:
        case ENEMY_FRAGMENT:
        case ENEMY_SWARM:
        case ENEMY_SPLITTER_CHILD:
            return GEM_TIER_S;
        case ENEMY_PACKET:
        case ENEMY_BADSECTOR:
            return GEM_TIER_L;
        default:
            return GEM_TIER_M;
    }
}

void weapon_kill_enemy(GameState *gs, int idx) {
    Enemy *e = &gs->enemies[idx];
    Vector2 pos = e->pos;
    EnemyType type = e->type;
    bool was_elite = e->is_elite;

    int burst = was_elite ? 30 : 10;
    particles_spawn_burst(gs, pos, enemy_color_for_type(type), burst);
    shake_add(gs, was_elite ? SHAKE_BOSS_HIT : SHAKE_KILL);

    gem_spawn_tier(gs, pos, gem_tier_for_enemy(type, was_elite));
    if (was_elite) {
        chest_drop(gs, pos);
        for (int i = 0; i < 5; i++) {
            float angle = (2.0f * 3.14159f * i) / 5.0f;
            Vector2 gp = {pos.x + cosf(angle) * 18.0f,
                          pos.y + sinf(angle) * 18.0f};
            gem_spawn(gs, gp);
        }
    } else {
        item_drop_roll(gs, type, pos);
    }
    e->active = false;
    gs->kills++;
    audio_play(SFX_ENEMY_DIE);

    if (type == ENEMY_SPLITTER) {
        for (int i = 0; i < SPLITTER_CHILD_COUNT; i++) {
            float angle = (2.0f * 3.14159f * i) / SPLITTER_CHILD_COUNT;
            Vector2 child_pos = {
                pos.x + cosf(angle) * 15.0f,
                pos.y + sinf(angle) * 15.0f
            };
            enemy_spawn_at(gs, ENEMY_SPLITTER_CHILD, child_pos);
        }
    } else if (type == ENEMY_BOMBER) {
        particles_spawn_burst(gs, pos, (Color){255, 100, 200, 255}, 24);
        shake_add(gs, SHAKE_HIT);
        float dx = gs->player.pos.x - pos.x;
        float dy = gs->player.pos.y - pos.y;
        if (sqrtf(dx * dx + dy * dy) < BOMBER_EXPLOSION_RADIUS + PLAYER_RADIUS) {
            player_take_damage(gs, BOMBER_EXPLOSION_DAMAGE);
        }
    }
}

bool weapon_hit_enemy(GameState *gs, int idx, int base_dmg, Color popup_col) {
    if (idx < 0 || idx >= MAX_ENEMIES) return false;
    Enemy *e = &gs->enemies[idx];
    if (!e->active) return false;
    int dmg = base_dmg + gs->weapon_damage_bonus;
    e->hp -= dmg;
    popup_spawn(gs, e->pos, dmg, popup_col);
    if (e->hp <= 0) {
        weapon_kill_enemy(gs, idx);
        return true;
    }
    return false;
}

void weapon_hit_boss(GameState *gs, int base_dmg) {
    if (!gs->boss.active) return;
    boss_take_damage(gs, base_dmg + gs->weapon_damage_bonus);
}

void weapon_aoe_damage(GameState *gs, Vector2 center, float radius,
                       int base_dmg, Color popup_col) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!gs->enemies[i].active) continue;
        if (gs->enemies[i].phased) continue;
        float dx = gs->enemies[i].pos.x - center.x;
        float dy = gs->enemies[i].pos.y - center.y;
        float reach = radius + gs->enemies[i].radius;
        if (dx * dx + dy * dy < reach * reach) {
            weapon_hit_enemy(gs, i, base_dmg, popup_col);
        }
    }
    weapon_try_hit_boss_radius(gs, center, radius, base_dmg);
    particles_spawn_burst(gs, center, popup_col, 12);
}

void weapon_try_hit_boss_radius(GameState *gs, Vector2 center,
                                 float radius, int base_dmg) {
    if (!gs->boss.active) return;
    float dx = gs->boss.pos.x - center.x;
    float dy = gs->boss.pos.y - center.y;
    float reach = radius + BOSS_RADIUS;
    if (dx * dx + dy * dy < reach * reach) {
        weapon_hit_boss(gs, base_dmg);
    }
}

int weapon_nearest_enemy(const GameState *gs, Vector2 from, float max_range) {
    int nearest = -1;
    float md = (max_range > 0) ? max_range * max_range : FLT_MAX;
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

bool weapon_nearest_target(const GameState *gs, Vector2 from, Vector2 *out_pos) {
    int idx = weapon_nearest_enemy(gs, from, 0);
    float best = FLT_MAX;
    Vector2 best_pos = {0, 0};
    bool found = false;

    if (idx >= 0) {
        best_pos = gs->enemies[idx].pos;
        float dx = best_pos.x - from.x;
        float dy = best_pos.y - from.y;
        best = dx * dx + dy * dy;
        found = true;
    }
    if (gs->boss.active) {
        float dx = gs->boss.pos.x - from.x;
        float dy = gs->boss.pos.y - from.y;
        float d2 = dx * dx + dy * dy;
        if (d2 < best) {
            best_pos = gs->boss.pos;
            found = true;
        }
    }
    if (found && out_pos) *out_pos = best_pos;
    return found;
}
