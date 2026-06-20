#ifndef CONFIG_H
#define CONFIG_H

// Logical resolution (scale to actual window)
#define LOGICAL_W 640
#define LOGICAL_H 400

// Window
#define WINDOW_W 960
#define WINDOW_H 600
#define TARGET_FPS 60

// Player
#define PLAYER_SPEED 168.0f
#define PLAYER_RADIUS 12.0f
#define PLAYER_MAX_HP 5
#define PLAYER_INVINCIBLE_TIME 0.8f
// Damage dealt back to the enemy/boss on contact (only when the hit
// actually landed — invincibility frames skip this too).
#define PLAYER_CONTACT_DAMAGE 3

// Weapon (Pulse Bolt)
#define WEAPON_FIRE_INTERVAL 0.56f
#define BULLET_SPEED 370.0f
#define BULLET_DAMAGE 1
#define BULLET_RADIUS 4.0f
#define MAX_BULLETS 256

// Orbiters
#define ORBITER_COUNT_BASE 2
#define ORBITER_RADIUS 8.0f
#define ORBITER_ORBIT_RADIUS 50.0f
#define ORBITER_SPEED 3.0f
#define ORBITER_DAMAGE 1
#define MAX_ORBITERS 8

// Beam
#define BEAM_INTERVAL 2.5f
#define BEAM_DURATION 0.8f
#define BEAM_LENGTH 300.0f
#define BEAM_WIDTH 8.0f
#define BEAM_DAMAGE 2
#define BEAM_SWEEP_ANGLE 0.7f

// Nova
#define NOVA_INTERVAL 3.0f
#define NOVA_RADIUS_BASE 80.0f
#define NOVA_DAMAGE 1
#define NOVA_EXPAND_SPEED 400.0f

// Spark Mines
#define MINE_INTERVAL 2.5f
#define MINE_LIFE 8.0f
#define MINE_RADIUS 10.0f
#define MINE_EXPLOSION_RADIUS 40.0f
#define MINE_DAMAGE 2
#define MAX_MINES 16

// Chain Lightning
#define CHAIN_INTERVAL 2.0f
#define CHAIN_RANGE 120.0f
#define CHAIN_JUMP_RANGE 80.0f
#define CHAIN_JUMPS 3
#define CHAIN_DAMAGE 1
#define CHAIN_VISUAL_LIFE 0.25f
#define CHAIN_MAX_POINTS 8

// Boomerang
#define BOOMERANG_INTERVAL 1.5f
#define BOOMERANG_SPEED 280.0f
#define BOOMERANG_RANGE 180.0f
#define BOOMERANG_RADIUS 10.0f
#define BOOMERANG_DAMAGE 1
#define MAX_BOOMERANGS 8

// Trail
#define TRAIL_INTERVAL 0.12f
#define TRAIL_LIFE 1.2f
#define TRAIL_RADIUS 8.0f
#define TRAIL_DAMAGE 1
#define MAX_TRAIL_MARKS 64

// Whip
#define WHIP_INTERVAL 1.4f
#define WHIP_ANIM 0.25f
#define WHIP_RANGE 70.0f
#define WHIP_ARC 2.0f
#define WHIP_DAMAGE 2

// Items & Chests
#define MAX_ITEMS 32
#define MAX_CHESTS 8
#define ITEM_RADIUS 9.0f
#define ITEM_LIFE 15.0f
#define CHEST_RADIUS 14.0f
#define CHEST_LIFE 25.0f
#define ITEM_DROP_CHANCE 14
#define ITEM_HP_HEAL 2

// Elite (mini-boss) spawns
#define ELITE_FIRST_TIME 45.0f
#define ELITE_INTERVAL 55.0f
#define ELITE_HP_MULT 5
#define ELITE_RADIUS_MULT 1.6f

// Formation events (ring/line of enemies)
#define FORMATION_FIRST_TIME 75.0f
#define FORMATION_INTERVAL 70.0f
#define FORMATION_RING_COUNT 12
#define FORMATION_RING_RADIUS 220.0f
#define FORMATION_LINE_COUNT 10

// Enemy
#define ENEMY_BASE_HP 2
#define ENEMY_BASE_SPEED 54.0f
#define ENEMY_SPEED_TIME_BONUS 40.0f
#define ENEMY_RADIUS 10.0f
#define ENEMY_DAMAGE 1
#define MAX_ENEMIES 256

// Enemy types
#define BIT_HP 2
#define BIT_SPEED 54.0f
#define BIT_RADIUS 10.0f

#define FRAGMENT_HP 1
#define FRAGMENT_SPEED 90.0f
#define FRAGMENT_RADIUS 7.0f

#define PACKET_HP 6
#define PACKET_SPEED 30.0f
#define PACKET_RADIUS 14.0f

#define GLITCH_HP 2
#define GLITCH_SPEED 60.0f
#define GLITCH_RADIUS 9.0f
#define GLITCH_DIR_CHANGE_TIME 0.4f

#define SPLITTER_HP 3
#define SPLITTER_SPEED 45.0f
#define SPLITTER_RADIUS 12.0f
#define SPLITTER_CHILD_COUNT 3

#define BOMBER_HP 2
#define BOMBER_SPEED 50.0f
#define BOMBER_RADIUS 11.0f
#define BOMBER_EXPLOSION_RADIUS 45.0f
#define BOMBER_EXPLOSION_DAMAGE 1

#define RANGER_HP 2
#define RANGER_SPEED 35.0f
#define RANGER_RADIUS 9.0f
#define RANGER_KEEP_DIST 130.0f
#define RANGER_FIRE_INTERVAL 1.8f

#define SWARM_HP 1
#define SWARM_SPEED 95.0f
#define SWARM_RADIUS 5.0f
#define SWARM_GROUP_SIZE 5

#define BADSECTOR_HP 10
#define BADSECTOR_RADIUS 16.0f
#define BADSECTOR_FIRE_INTERVAL 2.2f

#define PHASER_HP 3
#define PHASER_SPEED 55.0f
#define PHASER_RADIUS 10.0f
#define PHASER_CYCLE 1.4f
#define PHASER_PHASE_RATIO 0.4f

#define TRACKER_HP 2
#define TRACKER_SPEED 70.0f
#define TRACKER_RADIUS 9.0f
#define TRACKER_LEAD_TIME 0.5f

// Enemy bullets
#define MAX_ENEMY_BULLETS 128
#define ENEMY_BULLET_SPEED 140.0f
#define ENEMY_BULLET_RADIUS 4.0f
#define ENEMY_BULLET_DAMAGE 1

// Spawning
#define SPAWN_INTERVAL_INITIAL 0.95f
#define SPAWN_INTERVAL_MIN 0.28f
#define SPAWN_MARGIN 40.0f

// Gems / XP
#define GEM_RADIUS 6.0f
#define GEM_PICKUP_RANGE 48.0f
#define MAX_GEMS 512
#define XP_BASE_REQUIREMENT 5
#define XP_PER_LEVEL 3
// Per-tier XP values
#define GEM_XP_S 1
#define GEM_XP_M 3
#define GEM_XP_L 7

// Upgrades
#define UPGRADE_CHOICES 3
#define UPGRADE_RAPID_FIRE_MULT 0.82f
#define UPGRADE_MULTI_SHOT_ADD 1
#define UPGRADE_POWER_ADD 1
#define UPGRADE_SPEED_MULT 1.12f
#define UPGRADE_MAGNET_ADD 34.0f
#define UPGRADE_VITALITY_ADD 1

// Global passives
#define UPGRADE_AREA_MULT_ADD 0.15f       // +15% AoE per pick
#define UPGRADE_DURATION_MULT_ADD 0.20f   // +20% duration per pick

// Weapon upgrades (one per weapon)
#define UPGRADE_ORBITER_COUNT_ADD 1
#define UPGRADE_BEAM_ARC_ADD 0.30f       // radians added to sweep half-angle
#define UPGRADE_NOVA_RANGE_ADD 30.0f
#define UPGRADE_MINE_BLAST_ADD 20.0f
#define UPGRADE_CHAIN_JUMPS_ADD 1
#define UPGRADE_BOOMERANG_SPIN_ADD 6.0f
#define UPGRADE_TRAIL_DURATION_ADD 0.8f
#define UPGRADE_WHIP_ARC_ADD 0.5f

// Max picks per upgrade (weapon unlocks are implicitly capped at 1)
#define UPGRADE_MAX_PICKS 8

// Game
#define GAME_DURATION 300.0f
#define DT_MAX 0.04f

// Difficulty (0..100). Maps to the 8 param multipliers via params_from_difficulty().
// CLI / debug can override at runtime, but this is the default for normal play.
#define DIFFICULTY 50
#define DIFFICULTY_EASY    20
#define DIFFICULTY_NORMAL  50
#define DIFFICULTY_HARD    75
#define DIFFICULTY_BRUTAL  90

// Particles
#define MAX_PARTICLES 512
#define PARTICLE_LIFE 0.5f
#define PARTICLE_SPEED 180.0f

// Popups (damage numbers, etc.)
#define MAX_POPUPS 64
#define POPUP_LIFE 0.7f
#define POPUP_RISE_SPEED 60.0f

// Flash
#define FLASH_DECAY 3.0f

// Screen shake
#define SHAKE_HIT 4.0f
#define SHAKE_KILL 2.0f
#define SHAKE_BOSS_HIT 6.0f
#define SHAKE_DECAY 8.0f
#define BOSS_SPAWN_TIME 240.0f

// Boss FORMAT
#define BOSS_HP 150
#define BOSS_SPEED 35.0f
#define BOSS_RADIUS 40.0f
#define BOSS_DAMAGE 2
#define BOSS_CHARGE_SPEED 200.0f
#define BOSS_CHARGE_DURATION 0.8f
#define BOSS_CHARGE_COOLDOWN 4.0f
#define BOSS_SPAWN_INTERVAL 3.0f

#endif
