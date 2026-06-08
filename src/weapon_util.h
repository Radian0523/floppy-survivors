#ifndef WEAPON_UTIL_H
#define WEAPON_UTIL_H

#include "game.h"

// === Damage helpers ===
// All apply the global weapon_damage_bonus to base_dmg automatically.
// Caller passes the BASE damage (e.g. gs->bullet_damage).

// Apply damage to one enemy. Spawns popup, handles death (kill_enemy).
// Returns true if the enemy died.
bool weapon_hit_enemy(GameState *gs, int idx, int base_dmg, Color popup_col);

// Apply damage to the boss (if active).
void weapon_hit_boss(GameState *gs, int base_dmg);

// AoE: damage every enemy (and boss) within radius from center.
// Spawns a particle burst at the center.
void weapon_aoe_damage(GameState *gs, Vector2 center, float radius,
                       int base_dmg, Color popup_col);

// Try to damage boss if within (radius + BOSS_RADIUS) from center.
void weapon_try_hit_boss_radius(GameState *gs, Vector2 center,
                                float radius, int base_dmg);

// Destroy any enemy bullets within (radius + ENEMY_BULLET_RADIUS) from center.
// Spawns a small particle burst per destroyed bullet. Returns number destroyed.
int weapon_destroy_bullets_at(GameState *gs, Vector2 center, float radius);

// === Targeting ===

// Nearest active, non-phased enemy within max_range from `from`.
// Returns enemy index, or -1 if none.
int weapon_nearest_enemy(const GameState *gs, Vector2 from, float max_range);

// Nearest active target (enemy or boss). Writes position to *out_pos.
// Returns true if found.
bool weapon_nearest_target(const GameState *gs, Vector2 from, Vector2 *out_pos);

// === Enemy color helper (also used by kill effects) ===
Color enemy_color_for_type(EnemyType type);

// === Internal: kill an enemy (called by weapon_hit_enemy on death). ===
void weapon_kill_enemy(GameState *gs, int idx);

#endif
