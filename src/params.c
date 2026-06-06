#include "params.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ParamSet g_params;

void params_defaults(ParamSet *p) {
    p->enemy_hp_mult = 1.0f;
    p->enemy_spawn_min_mult = 1.0f;
    p->enemy_speed_bonus_mult = 1.0f;
    p->enemy_damage_mult = 1.0f;
    p->spawn_count_mult = 1.0f;
    p->player_speed_mult = 1.0f;
    p->player_hp_mult = 1.0f;
    p->player_invincible_mult = 1.0f;
}

static float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

// Pivot curve: d=0 -> min_v, d=50 -> 1.0, d=100 -> max_v.
static float pivot(float d, float min_v, float max_v) {
    if (d <= 50.0f) return lerp(min_v, 1.0f, d / 50.0f);
    return lerp(1.0f, max_v, (d - 50.0f) / 50.0f);
}

void params_from_difficulty(ParamSet *p, float difficulty) {
    if (difficulty < 0) difficulty = 0;
    if (difficulty > 100) difficulty = 100;

    // d=50 -> all 1.0 (vanilla). d=0 -> player-favored. d=100 -> challenging.
    // Compounding effect: small per-multiplier shifts combine into large
    // overall changes. Keep each axis modest.
    p->enemy_hp_mult          = pivot(difficulty, 0.55f, 1.70f);
    p->enemy_spawn_min_mult   = pivot(difficulty, 1.80f, 0.65f);
    p->enemy_speed_bonus_mult = pivot(difficulty, 0.50f, 1.50f);
    p->enemy_damage_mult      = pivot(difficulty, 0.70f, 1.40f);
    p->spawn_count_mult       = pivot(difficulty, 0.60f, 1.45f);
    p->player_speed_mult      = pivot(difficulty, 1.25f, 0.88f);
    p->player_hp_mult         = pivot(difficulty, 1.80f, 0.80f);
    p->player_invincible_mult = pivot(difficulty, 1.40f, 0.82f);
}

static bool set_param(ParamSet *p, const char *key, float val) {
    if (strcmp(key, "enemy_hp_mult") == 0) p->enemy_hp_mult = val;
    else if (strcmp(key, "enemy_spawn_min_mult") == 0) p->enemy_spawn_min_mult = val;
    else if (strcmp(key, "enemy_speed_bonus_mult") == 0) p->enemy_speed_bonus_mult = val;
    else if (strcmp(key, "enemy_damage_mult") == 0) p->enemy_damage_mult = val;
    else if (strcmp(key, "spawn_count_mult") == 0) p->spawn_count_mult = val;
    else if (strcmp(key, "player_speed_mult") == 0) p->player_speed_mult = val;
    else if (strcmp(key, "player_hp_mult") == 0) p->player_hp_mult = val;
    else if (strcmp(key, "player_invincible_mult") == 0) p->player_invincible_mult = val;
    else return false;
    return true;
}

bool params_apply_kv(ParamSet *p, const char *kv_string) {
    if (!kv_string) return true;
    char buf[1024];
    strncpy(buf, kv_string, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = 0;

    char *tok = strtok(buf, ",");
    while (tok) {
        char *eq = strchr(tok, '=');
        if (!eq) {
            fprintf(stderr, "param: missing '=' in '%s'\n", tok);
            return false;
        }
        *eq = 0;
        const char *key = tok;
        float val = (float)atof(eq + 1);
        if (!set_param(p, key, val)) {
            fprintf(stderr, "param: unknown key '%s'\n", key);
            return false;
        }
        tok = strtok(NULL, ",");
    }
    return true;
}

static const char *opt_arg(const char *flag, const char *arg) {
    size_t n = strlen(flag);
    if (strncmp(arg, flag, n) != 0) return NULL;
    if (arg[n] == '=') return arg + n + 1;
    return NULL;
}

void cli_parse(int argc, char **argv, CliOptions *opt) {
    opt->headless = false;
    opt->bot = false;
    opt->seed = 0;
    opt->seed_set = false;
    opt->duration_override = 0;
    opt->duration_set = false;
    opt->output_path = NULL;
    opt->params_arg = NULL;

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        const char *v;
        if (strcmp(a, "--headless") == 0) opt->headless = true;
        else if (strcmp(a, "--bot") == 0) opt->bot = true;
        else if ((v = opt_arg("--seed", a))) {
            opt->seed = (unsigned)strtoul(v, NULL, 10);
            opt->seed_set = true;
        }
        else if ((v = opt_arg("--duration", a))) {
            opt->duration_override = (float)atof(v);
            opt->duration_set = true;
        }
        else if ((v = opt_arg("--output", a))) opt->output_path = v;
        else if ((v = opt_arg("--params", a))) opt->params_arg = v;
        else {
            fprintf(stderr, "Unknown option: %s\n", a);
        }
    }
}
