#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "config.h"
#include <stdbool.h>

typedef enum {
    UPGRADE_RAPID_FIRE,
    UPGRADE_MULTI_SHOT,
    UPGRADE_POWER,
    UPGRADE_SPEED,
    UPGRADE_MAGNET,
    UPGRADE_VITALITY,
    UPGRADE_ORBITERS,
    UPGRADE_BEAM,
    UPGRADE_NOVA,
    UPGRADE_COUNT
} UpgradeType;

typedef struct {
    Vector2 pos;
    Vector2 vel;
    float facing_angle;
    int hp;
    int max_hp;
    float invincible_timer;
    float speed;
    float pickup_range;
} Player;

typedef struct {
    Vector2 pos;
    Vector2 vel;
    bool active;
} Bullet;

typedef struct {
    float angle;
    bool active;
} Orbiter;

typedef struct {
    float angle;
    float center_angle;
    float timer;
    bool firing;
    float fire_timer;
} BeamState;

typedef struct {
    float timer;
    float current_radius;
    bool expanding;
} NovaState;

typedef enum {
    ENEMY_BIT,
    ENEMY_FRAGMENT,
    ENEMY_PACKET,
    ENEMY_GLITCH,
    ENEMY_SPLITTER,
    ENEMY_SPLITTER_CHILD,
    ENEMY_TYPE_COUNT
} EnemyType;

typedef struct {
    Vector2 pos;
    Vector2 vel;
    int hp;
    float speed;
    float radius;
    EnemyType type;
    float glitch_timer;
    bool active;
} Enemy;

typedef struct {
    Vector2 pos;
    bool active;
} Gem;

typedef struct {
    Player player;
    Bullet bullets[MAX_BULLETS];
    Enemy enemies[MAX_ENEMIES];
    Gem gems[MAX_GEMS];

    int xp;
    int level;
    int xp_to_next;

    // Pulse Bolt
    float fire_timer;
    float fire_interval;
    int bullet_count;
    int bullet_damage;

    // Orbiters
    bool has_orbiters;
    Orbiter orbiters[MAX_ORBITERS];
    int orbiter_count;
    int orbiter_damage;
    float orbiter_orbit_radius;

    // Beam
    bool has_beam;
    BeamState beam;
    float beam_interval;
    int beam_damage;
    float beam_length;

    // Nova
    bool has_nova;
    NovaState nova;
    float nova_interval;
    int nova_damage;
    float nova_max_radius;

    float spawn_timer;
    float spawn_interval;

    float game_time;
    bool game_over;
    bool victory;

    int kills;

    bool upgrading;
    UpgradeType upgrade_choices[UPGRADE_CHOICES];
    int upgrade_hover;

    float scale;
    Vector2 offset;
} GameState;

// Module functions
void player_init(Player *p, float scale);
void player_update(Player *p, float dt, float scale);
void player_take_damage(Player *p, int damage);
void player_draw(const Player *p, float scale, Vector2 offset);

void weapon_update(GameState *gs, float dt);
void bullet_update(GameState *gs, float dt);
void bullet_draw(const Bullet bullets[], float scale, Vector2 offset);

void orbiters_init(GameState *gs);
void orbiters_update(GameState *gs, float dt);
void orbiters_draw(const GameState *gs, float scale, Vector2 offset);

void beam_init(GameState *gs);
void beam_update(GameState *gs, float dt);
void beam_draw(const GameState *gs, float scale, Vector2 offset);

void nova_init(GameState *gs);
void nova_update(GameState *gs, float dt);
void nova_draw(const GameState *gs, float scale, Vector2 offset);

void enemy_spawn(GameState *gs);
void enemy_spawn_at(GameState *gs, EnemyType type, Vector2 pos);
void enemy_update(GameState *gs, float dt);
void enemy_draw(const Enemy enemies[], float scale, Vector2 offset);

void gem_spawn(GameState *gs, Vector2 pos);
void gem_update(GameState *gs, float dt);
void gem_draw(const Gem gems[], float scale, Vector2 offset);

void render_background(void);

void upgrade_start(GameState *gs);
void upgrade_update(GameState *gs);
void upgrade_draw(const GameState *gs);
const char* upgrade_get_name(UpgradeType type);

Vector2 input_get_move_direction(void);
bool input_is_mouse_active(void);
Vector2 input_get_mouse_direction(Vector2 player_screen_pos);

#endif
