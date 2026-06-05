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

// Spawning
#define SPAWN_INTERVAL_INITIAL 0.95f
#define SPAWN_INTERVAL_MIN 0.28f
#define SPAWN_MARGIN 40.0f

// Gems / XP
#define GEM_XP_VALUE 1
#define GEM_RADIUS 6.0f
#define GEM_PICKUP_RANGE 48.0f
#define MAX_GEMS 512
#define XP_BASE_REQUIREMENT 5
#define XP_PER_LEVEL 3

// Upgrades
#define UPGRADE_CHOICES 3
#define UPGRADE_RAPID_FIRE_MULT 0.82f
#define UPGRADE_MULTI_SHOT_ADD 1
#define UPGRADE_POWER_ADD 1
#define UPGRADE_SPEED_MULT 1.12f
#define UPGRADE_MAGNET_ADD 34.0f
#define UPGRADE_VITALITY_ADD 1

// Game
#define GAME_DURATION 300.0f
#define DT_MAX 0.04f

#endif
