#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "config.h"
#include "params.h"
#include <stdbool.h>

typedef enum {
    SCENE_TITLE,
    SCENE_WEAPON_SELECT,
    SCENE_GAME,
    SCENE_RESULT
} GameScene;

typedef enum {
    STARTING_WEAPON_PULSE,
    STARTING_WEAPON_ORBITERS,
    STARTING_WEAPON_BEAM,
    STARTING_WEAPON_NOVA,
    STARTING_WEAPON_COUNT
} StartingWeapon;

typedef enum {
    // Base upgrades
    UPGRADE_RAPID_FIRE,
    UPGRADE_MULTI_SHOT,
    UPGRADE_POWER,
    UPGRADE_SPEED,
    UPGRADE_MAGNET,
    UPGRADE_VITALITY,
    // Weapon unlocks
    UPGRADE_ORBITERS,
    UPGRADE_BEAM,
    UPGRADE_NOVA,
    UPGRADE_MINES,
    UPGRADE_CHAIN,
    UPGRADE_BOOMERANG,
    UPGRADE_TRAIL,
    UPGRADE_WHIP,
    // Orbiters upgrades
    UPGRADE_ORBITER_COUNT,
    UPGRADE_ORBITER_RADIUS,
    // Beam upgrades
    UPGRADE_BEAM_WIDTH,
    UPGRADE_BEAM_INTERVAL,
    // Nova upgrades
    UPGRADE_NOVA_RANGE,
    UPGRADE_NOVA_DAMAGE,
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

typedef struct {
    Vector2 pos;
    float life;
    bool active;
} Mine;

typedef struct {
    Vector2 points[CHAIN_MAX_POINTS];
    int count;
    float life;
} ChainVisual;

typedef struct {
    Vector2 pos;
    Vector2 dir;
    float traveled;
    bool returning;
    bool active;
} BoomerangProj;

typedef struct {
    Vector2 pos;
    float life;
    bool active;
} TrailMark;

typedef enum {
    ENEMY_BIT,
    ENEMY_FRAGMENT,
    ENEMY_PACKET,
    ENEMY_GLITCH,
    ENEMY_SPLITTER,
    ENEMY_SPLITTER_CHILD,
    ENEMY_BOMBER,
    ENEMY_RANGER,
    ENEMY_SWARM,
    ENEMY_BADSECTOR,
    ENEMY_PHASER,
    ENEMY_TRACKER,
    ENEMY_TYPE_COUNT
} EnemyType;

typedef struct {
    Vector2 pos;
    Vector2 vel;
    int hp;
    float speed;
    float radius;
    EnemyType type;
    float type_timer;
    float phase_timer;
    bool phased;
    bool is_elite;
    bool active;
} Enemy;

typedef struct {
    Vector2 pos;
    Vector2 vel;
    bool active;
} EnemyBullet;

typedef struct {
    Vector2 pos;
    bool active;
} Gem;

typedef struct {
    float best_time;
    int best_kills;
    int best_level;
    int boss_defeats;
    int total_games;
} BestScore;

typedef enum {
    ITEM_HP,
    ITEM_MAGNET,
    ITEM_TYPE_COUNT
} ItemType;

typedef struct {
    Vector2 pos;
    ItemType type;
    float life;
    bool active;
} Item;

typedef struct {
    Vector2 pos;
    float life;
    bool active;
} Chest;

typedef struct {
    Vector2 pos;
    Vector2 vel;
    float life;
    float max_life;
    Color color;
    bool active;
} Particle;

typedef struct {
    Vector2 pos;
    float life;
    float max_life;
    int value;
    Color color;
    bool active;
} Popup;

typedef struct {
    Vector2 pos;
    Vector2 vel;
    int hp;
    int max_hp;
    bool active;
    bool charging;
    float charge_timer;
    float charge_cooldown;
    float spawn_timer;
    Vector2 charge_dir;
} Boss;

typedef struct {
    Player player;
    Bullet bullets[MAX_BULLETS];
    Enemy enemies[MAX_ENEMIES];
    Gem gems[MAX_GEMS];

    int xp;
    int level;
    int xp_to_next;

    // Pulse Bolt
    bool has_pulse_bolt;
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
    float beam_width;

    // Nova
    bool has_nova;
    NovaState nova;
    float nova_interval;
    int nova_damage;
    float nova_max_radius;

    // Spark Mines
    bool has_mines;
    Mine mines[MAX_MINES];
    float mine_timer;
    float mine_interval;
    int mine_damage;

    // Chain Lightning
    bool has_chain;
    float chain_timer;
    float chain_interval;
    int chain_jumps;
    int chain_damage;
    ChainVisual chain_visual;

    // Boomerang
    bool has_boomerang;
    BoomerangProj boomerangs[MAX_BOOMERANGS];
    float boomerang_timer;
    float boomerang_interval;
    int boomerang_damage;

    // Trail
    bool has_trail;
    TrailMark trail[MAX_TRAIL_MARKS];
    float trail_timer;
    int trail_damage;

    // Whip
    bool has_whip;
    float whip_timer;
    float whip_anim;
    float whip_interval;
    int whip_damage;

    float spawn_timer;
    float spawn_interval;
    float elite_timer;
    float formation_timer;

    float game_time;
    bool game_over;
    bool victory;

    int kills;

    Boss boss;
    bool boss_defeated;

    Particle particles[MAX_PARTICLES];
    Popup popups[MAX_POPUPS];
    EnemyBullet enemy_bullets[MAX_ENEMY_BULLETS];
    Item items[MAX_ITEMS];
    Chest chests[MAX_CHESTS];
    float magnet_pull_timer;
    float shake_amount;
    float flash_amount;
    Color flash_color;

    GameScene scene;
    float scene_timer;
    int weapon_select_hover;
    int difficulty;
    bool paused;

    BestScore best;
    int last_score_flags;

    bool debug_open;
    bool debug_invincible;
    bool bot_mode;
    bool headless;

    bool upgrading;
    UpgradeType upgrade_choices[UPGRADE_CHOICES];
    int upgrade_hover;

    float scale;
    Vector2 offset;
} GameState;

// Module functions
void player_init(Player *p, float scale);
void player_update(GameState *gs, float dt, float scale);
void player_take_damage(GameState *gs, int damage);
void player_draw(const Player *p, float scale, Vector2 offset);
void player_draw_hp_bar(const Player *p, float scale, Vector2 offset);

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

void mines_init(GameState *gs);
void mines_update(GameState *gs, float dt);
void mines_draw(const GameState *gs, float scale, Vector2 offset);

void chain_init(GameState *gs);
void chain_update(GameState *gs, float dt);
void chain_draw(const GameState *gs, float scale, Vector2 offset);

void boomerang_init(GameState *gs);
void boomerang_update(GameState *gs, float dt);
void boomerang_draw(const GameState *gs, float scale, Vector2 offset);

void trail_init(GameState *gs);
void trail_update(GameState *gs, float dt);
void trail_draw(const GameState *gs, float scale, Vector2 offset);

void whip_init(GameState *gs);
void whip_update(GameState *gs, float dt);
void whip_draw(const GameState *gs, float scale, Vector2 offset);

void enemy_spawn(GameState *gs);
void enemy_spawn_at(GameState *gs, EnemyType type, Vector2 pos);
void enemy_update(GameState *gs, float dt);
void enemy_draw(const Enemy enemies[], float scale, Vector2 offset);
void enemy_bullet_spawn(GameState *gs, Vector2 pos, Vector2 dir);
void enemy_bullets_update(GameState *gs, float dt);
void enemy_bullets_draw(const GameState *gs, float scale, Vector2 offset);

void items_init(GameState *gs);
void item_drop_roll(GameState *gs, EnemyType type, Vector2 pos);
void chest_drop(GameState *gs, Vector2 pos);
void enemy_spawn_elite_force(GameState *gs);
void enemy_spawn_formation_force(GameState *gs);

void debug_update(GameState *gs);
void debug_draw(const GameState *gs);

void upgrade_auto_pick(GameState *gs);
void items_update(GameState *gs, float dt);
void items_draw(const GameState *gs, float scale, Vector2 offset);
void chests_update(GameState *gs, float dt);
void chests_draw(const GameState *gs, float scale, Vector2 offset);

void boss_spawn(GameState *gs);
void boss_update(GameState *gs, float dt);
void boss_draw(const GameState *gs, float scale, Vector2 offset);
void boss_take_damage(GameState *gs, int damage);

void particles_init(GameState *gs);
void particles_spawn_burst(GameState *gs, Vector2 pos, Color color, int count);
void particles_update(GameState *gs, float dt);
void particles_draw(const GameState *gs, float scale, Vector2 offset);
void shake_add(GameState *gs, float amount);

void popup_spawn(GameState *gs, Vector2 pos, int value, Color color);
void popups_update(GameState *gs, float dt);
void popups_draw(const GameState *gs, float scale, Vector2 offset);
void flash_trigger(GameState *gs, Color color, float amount);
void flash_draw(const GameState *gs);

Vector2 bot_compute_direction(const GameState *gs);

void scene_title_update(GameState *gs, float dt);
void scene_title_draw(const GameState *gs);
void scene_title_draw_world(void);  // additive-blended background drifts
void scene_weapon_select_update(GameState *gs, float dt);
void scene_weapon_select_draw(const GameState *gs);
void scene_result_update(GameState *gs, float dt);
void scene_result_draw(const GameState *gs);
void pause_draw(const GameState *gs);

void game_start_with_weapon(GameState *gs, StartingWeapon weapon);

typedef enum {
    SFX_SHOOT,
    SFX_ENEMY_HIT,
    SFX_ENEMY_DIE,
    SFX_PLAYER_HIT,
    SFX_LEVEL_UP,
    SFX_GEM_PICKUP,
    SFX_BOSS_SPAWN,
    SFX_BOSS_HIT,
    SFX_NOVA,
    SFX_BEAM,
    SFX_COUNT
} SfxId;

void audio_init(void);
void audio_play(SfxId id);
void audio_cleanup(void);

typedef enum {
    BGM_NONE,
    BGM_TITLE,
    BGM_GAME,
    BGM_BOSS,
    BGM_COUNT
} BgmId;

void bgm_init(void);
void bgm_play(BgmId id);
void bgm_stop(void);
void bgm_cleanup(void);

void score_load(BestScore *s);
void score_save(const BestScore *s);
// Returns flags: bit0=time, bit1=kills, bit2=level, bit3=first_boss
int score_update(BestScore *s, float time, int kills, int level, bool boss_killed);

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
