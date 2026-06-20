#ifndef PARAMS_H
#define PARAMS_H

#include <stdbool.h>

typedef struct {
    float enemy_hp_mult;
    float enemy_spawn_min_mult;
    float enemy_speed_bonus_mult;
    float enemy_damage_mult;
    float spawn_count_mult;
    float player_speed_mult;
    float player_hp_mult;
    float player_invincible_mult;
} ParamSet;

typedef struct {
    bool headless;
    bool bot;
    bool all_weapons;     // grant all 9 weapons at start (for per-weapon DPS tuning)
    unsigned seed;
    bool seed_set;
    float duration_override;
    bool duration_set;
    const char *output_path;
    const char *params_arg;
} CliOptions;

void params_defaults(ParamSet *p);
bool params_apply_kv(ParamSet *p, const char *kv_string);
// Map difficulty (0..100) to all 8 multipliers using a hand-tuned curve.
void params_from_difficulty(ParamSet *p, float difficulty);

void cli_parse(int argc, char **argv, CliOptions *opt);

extern ParamSet g_params;

#endif
