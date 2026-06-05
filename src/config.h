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

// Enemy
#define ENEMY_BASE_HP 2
#define ENEMY_BASE_SPEED 54.0f
#define ENEMY_SPEED_TIME_BONUS 40.0f
#define ENEMY_RADIUS 10.0f
#define ENEMY_DAMAGE 1
#define MAX_ENEMIES 256

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

// Game
#define GAME_DURATION 300.0f
#define DT_MAX 0.04f

#endif
