#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "config.h"
#include "params.h"
#include <stdbool.h>

typedef enum {
    SCENE_TITLE,
    SCENE_HOW_TO_PLAY,
    SCENE_SETTINGS,
    SCENE_WEAPON_SELECT,
    SCENE_GAME,
    SCENE_RESULT
} GameScene;

typedef struct {
    float sfx_volume;   // 0.0 - 1.0
    float bgm_volume;
    bool fullscreen;
} Settings;

typedef enum {
    STARTING_WEAPON_PULSE,
    STARTING_WEAPON_ORBITERS,
    STARTING_WEAPON_BEAM,
    STARTING_WEAPON_NOVA,
    STARTING_WEAPON_COUNT
} StartingWeapon;

typedef enum {
    WEAPON_ID_PULSE,
    WEAPON_ID_ORBITERS,
    WEAPON_ID_BEAM,
    WEAPON_ID_NOVA,
    WEAPON_ID_MINES,
    WEAPON_ID_CHAIN,
    WEAPON_ID_BOOMERANG,
    WEAPON_ID_TRAIL,
    WEAPON_ID_WHIP,
    WEAPON_ID_COUNT
} WeaponID;

extern const char *WEAPON_NAMES[WEAPON_ID_COUNT];

typedef struct {
    int damage_dealt[WEAPON_ID_COUNT];
    int kills_by[WEAPON_ID_COUNT];
    int damage_taken;
    int gems_collected;
    int items_collected;
    int xp_collected;
} GameStats;

typedef enum {
    // Base upgrades
    UPGRADE_RAPID_FIRE,
    UPGRADE_MULTI_SHOT,
    UPGRADE_POWER,
    UPGRADE_SPEED,
    UPGRADE_MAGNET,
    UPGRADE_VITALITY,
    UPGRADE_AREA,
    UPGRADE_DURATION,
    // Weapon unlocks
    UPGRADE_ORBITERS,
    UPGRADE_BEAM,
    UPGRADE_NOVA,
    UPGRADE_MINES,
    UPGRADE_CHAIN,
    UPGRADE_BOOMERANG,
    UPGRADE_TRAIL,
    UPGRADE_WHIP,
    // Weapon-specific upgrades (one per weapon)
    UPGRADE_ORBITER_COUNT,    // Orbiters: +1 orb
    UPGRADE_BEAM_ARC,         // Beam: wider sweep angle
    UPGRADE_NOVA_RANGE,       // Nova: bigger radius
    UPGRADE_MINE_BLAST,       // Mines: bigger explosion
    UPGRADE_CHAIN_JUMPS,      // Chain: more jumps
    UPGRADE_BOOMERANG_SPIN,   // Boomerang: bigger hitbox
    UPGRADE_TRAIL_DURATION,   // Trail: longer life
    UPGRADE_WHIP_ARC,         // Whip: wider arc
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

// === Per-slot types ===
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

// === Weapon containers ===
typedef struct {
    bool has;
    float fire_timer;
    float fire_interval;
    int bullet_count;
    int damage;
} PulseWeapon;

typedef struct {
    bool has;
    int count;
    int damage;
    float orbit_radius;
    float base_angle;  // single rotation phase; each orb sits at base + 2πi/count
} OrbitersWeapon;

typedef struct {
    bool has;
    float angle;
    float center_angle;
    float timer;
    bool firing;
    float fire_timer;
    float interval;
    int damage;
    float length;
    float width;
    float sweep_angle;   // +- half-angle covered during one sweep
} BeamWeapon;

typedef struct {
    bool has;
    float timer;
    float current_radius;
    bool expanding;
    float interval;
    int damage;
    float max_radius;
} NovaWeapon;

typedef struct {
    bool has;
    Mine slots[MAX_MINES];
    float timer;
    float interval;
    int damage;
    float explosion_radius;
} MinesWeapon;

typedef struct {
    bool has;
    float timer;
    float interval;
    int jumps;
    int damage;
    ChainVisual visual;
} ChainWeapon;

typedef struct {
    bool has;
    BoomerangProj slots[MAX_BOOMERANGS];
    float timer;
    float interval;
    int damage;
    float radius;
} BoomerangWeapon;

typedef struct {
    bool has;
    TrailMark slots[MAX_TRAIL_MARKS];
    float timer;
    int damage;
    float life;
} TrailWeapon;

typedef struct {
    bool has;
    float timer;
    float anim;
    float interval;
    int damage;
    float arc;
} WhipWeapon;

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
    // Per-weapon last-hit timestamps for hit cooldown (persistent contact weapons).
    // Indexed by WeaponID. Sentinel WEAPON_ID_COUNT (contact damage) bypasses cooldown.
    float last_hit_time[16];  // sized >= WEAPON_ID_COUNT, fixed to avoid forward ref
} Enemy;

typedef struct {
    Vector2 pos;
    Vector2 vel;
    bool active;
} EnemyBullet;

typedef enum {
    GEM_TIER_S,
    GEM_TIER_M,
    GEM_TIER_L,
    GEM_TIER_COUNT
} GemTier;

typedef struct {
    Vector2 pos;
    GemTier tier;
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
    float last_hit_time[16];  // per-weapon hit cooldown (mirrors Enemy.last_hit_time)
} Boss;

typedef struct {
    Player player;
    Bullet bullets[MAX_BULLETS];
    Enemy enemies[MAX_ENEMIES];
    Gem gems[MAX_GEMS];

    int xp;
    int level;
    int xp_to_next;

    // Global weapon modifiers (apply to ALL owned weapons)
    float weapon_rate_mult;        // RAPID FIRE: multiplies every weapon interval
    int weapon_damage_bonus;       // POWER: added to every weapon damage
    int weapon_extra_projectiles;  // MULTI SHOT: extra projectiles for projectile weapons
    float weapon_area_mult;        // AREA: multiplies AoE radii (Nova/Mine/Whip/Trail/Beam width)
    float weapon_duration_mult;    // DURATION: multiplies timed effects (Trail/Mine/Beam/Whip lifetimes)

    // Weapons (each in its own container struct)
    PulseWeapon pulse;
    OrbitersWeapon orbiters;
    BeamWeapon beam;
    NovaWeapon nova;
    MinesWeapon mines;
    ChainWeapon chain;
    BoomerangWeapon boomerang;
    TrailWeapon trail;
    WhipWeapon whip;

    float spawn_timer;
    float spawn_interval;
    float elite_timer;
    float formation_timer;

    float game_time;
    bool game_over;
    bool victory;

    int kills;
    GameStats stats;

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
    int game_speed;   // 1 = normal, 2 = 2x, 3 = 3x (in-game time multiplier)
    bool paused;
    Settings settings;
    int settings_hover;

    BestScore best;
    int last_score_flags;

    bool debug_open;
    bool debug_invincible;
    bool bot_mode;
    bool headless;

    bool upgrading;
    UpgradeType upgrade_choices[UPGRADE_CHOICES];
    int upgrade_hover;
    int upgrade_picks[UPGRADE_COUNT];

    float scale;
    Vector2 offset;
} GameState;

// Module functions
void player_init(Player *p, float scale);
void player_update(GameState *gs, float dt, float scale);
// Returns true if damage was actually applied (not invincible, not god mode).
bool player_take_damage(GameState *gs, int damage);
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
void scene_how_to_play_update(GameState *gs, float dt);
void scene_how_to_play_draw(const GameState *gs);
void scene_settings_update(GameState *gs, float dt);
void scene_settings_draw(const GameState *gs);

void settings_load(Settings *s);
void settings_save(const Settings *s);
void settings_apply(const Settings *s);
void audio_set_sfx_volume(float v);
void bgm_set_volume(float v);
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
    BGM_GAME_LATE,
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
void gem_spawn_tier(GameState *gs, Vector2 pos, GemTier tier);
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
