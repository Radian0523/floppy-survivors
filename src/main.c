#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static const char *bloomShaderCode =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "out vec4 finalColor;\n"
    "uniform sampler2D texture0;\n"
    "uniform vec2 resolution;\n"
    "void main() {\n"
    "    vec2 ts = 3.0 / resolution;\n"
    "    vec4 c = texture(texture0, fragTexCoord);\n"
    "    vec4 b = vec4(0.0);\n"
    "    for (int i = -6; i <= 6; i++) {\n"
    "        for (int j = -6; j <= 6; j++) {\n"
    "            float d = float(i*i + j*j);\n"
    "            float w = exp(-d / 18.0);\n"
    "            b += texture(texture0, fragTexCoord + vec2(float(i), float(j)) * ts) * w;\n"
    "        }\n"
    "    }\n"
    "    b /= 50.0;\n"
    "    finalColor = c + b * 1.2;\n"
    "}\n";

static void game_init(GameState *gs) {
    float scale_x = (float)GetScreenWidth() / LOGICAL_W;
    float scale_y = (float)GetScreenHeight() / LOGICAL_H;
    gs->scale = (scale_x < scale_y) ? scale_x : scale_y;
    gs->offset.x = (GetScreenWidth() - LOGICAL_W * gs->scale) / 2;
    gs->offset.y = (GetScreenHeight() - LOGICAL_H * gs->scale) / 2;

    player_init(&gs->player, gs->scale);

    for (int i = 0; i < MAX_BULLETS; i++) gs->bullets[i].active = false;
    for (int i = 0; i < MAX_ENEMIES; i++) gs->enemies[i].active = false;
    for (int i = 0; i < MAX_GEMS; i++) gs->gems[i].active = false;
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) gs->enemy_bullets[i].active = false;

    gs->xp = 0;
    gs->level = 1;
    gs->xp_to_next = XP_BASE_REQUIREMENT;

    gs->has_pulse_bolt = false;
    gs->fire_timer = 0;
    gs->fire_interval = WEAPON_FIRE_INTERVAL;
    gs->bullet_count = 1;
    gs->bullet_damage = BULLET_DAMAGE;

    gs->has_orbiters = false;
    gs->orbiter_count = 0;
    gs->has_beam = false;
    gs->has_nova = false;
    gs->has_mines = false;
    gs->has_chain = false;
    gs->has_boomerang = false;
    gs->has_trail = false;
    gs->has_whip = false;

    gs->spawn_timer = 1.0f;
    gs->spawn_interval = SPAWN_INTERVAL_INITIAL;
    gs->elite_timer = ELITE_FIRST_TIME;
    gs->formation_timer = FORMATION_FIRST_TIME;

    gs->game_time = 0;
    gs->game_over = false;
    gs->victory = false;
    gs->kills = 0;

    gs->boss.active = false;
    gs->boss_defeated = false;

    particles_init(gs);
    items_init(gs);
    gs->magnet_pull_timer = 0;

    gs->upgrading = false;
    gs->upgrade_hover = -1;
    gs->paused = false;
    gs->last_score_flags = 0;
}

void game_start_with_weapon(GameState *gs, StartingWeapon weapon) {
    game_init(gs);
    switch (weapon) {
        case STARTING_WEAPON_PULSE:
            gs->has_pulse_bolt = true;
            break;
        case STARTING_WEAPON_ORBITERS:
            orbiters_init(gs);
            break;
        case STARTING_WEAPON_BEAM:
            beam_init(gs);
            break;
        case STARTING_WEAPON_NOVA:
            nova_init(gs);
            break;
        default:
            gs->has_pulse_bolt = true;
            break;
    }
}

static void draw_hud(const GameState *gs) {
    char buf[64];

    int minutes = (int)gs->game_time / 60;
    int seconds = (int)gs->game_time % 60;
    sprintf(buf, "TIME %d:%02d", minutes, seconds);
    DrawText(buf, 10, 10, 20, (Color){200, 200, 200, 255});

    sprintf(buf, "HP %d/%d", gs->player.hp, gs->player.max_hp);
    DrawText(buf, 10, 35, 20, (Color){255, 100, 100, 255});

    sprintf(buf, "LV %d", gs->level);
    DrawText(buf, 10, 60, 20, (Color){100, 255, 255, 255});

    int bar_w = 150;
    int bar_h = 10;
    int bar_x = 60;
    int bar_y = 65;
    float xp_ratio = (float)gs->xp / gs->xp_to_next;
    DrawRectangle(bar_x, bar_y, bar_w, bar_h, (Color){30, 30, 50, 255});
    DrawRectangle(bar_x, bar_y, (int)(bar_w * xp_ratio), bar_h, (Color){100, 255, 100, 255});

    sprintf(buf, "KILLS %d", gs->kills);
    DrawText(buf, GetScreenWidth() - 120, 10, 20, (Color){200, 200, 200, 255});
}

static void update_game(GameState *gs, float dt) {
    debug_update(gs);

    if (gs->paused) {
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_SPACE)) {
            gs->paused = false;
        }
        if (IsKeyPressed(KEY_Q)) {
            gs->paused = false;
            gs->scene = SCENE_TITLE;
            gs->scene_timer = 0;
        }
        return;
    }

    if (gs->upgrading) {
        upgrade_update(gs);
        return;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        gs->paused = true;
        return;
    }

    gs->game_time += dt;

    if (!gs->boss.active && !gs->boss_defeated && gs->game_time >= BOSS_SPAWN_TIME) {
        boss_spawn(gs);
    }

    if (gs->game_time >= GAME_DURATION) {
        gs->game_over = true;
        gs->victory = true;
        gs->scene = SCENE_RESULT;
        gs->scene_timer = 0;
        gs->last_score_flags = score_update(&gs->best, gs->game_time, gs->kills,
            gs->level, gs->boss_defeated);
        return;
    }

    if (gs->player.hp <= 0) {
        gs->game_over = true;
        gs->victory = false;
        gs->scene = SCENE_RESULT;
        gs->scene_timer = 0;
        gs->last_score_flags = score_update(&gs->best, gs->game_time, gs->kills,
            gs->level, false);
        return;
    }

    player_update(gs, dt, gs->scale);
    weapon_update(gs, dt);
    bullet_update(gs, dt);
    orbiters_update(gs, dt);
    beam_update(gs, dt);
    nova_update(gs, dt);
    mines_update(gs, dt);
    chain_update(gs, dt);
    boomerang_update(gs, dt);
    trail_update(gs, dt);
    whip_update(gs, dt);
    enemy_update(gs, dt);
    enemy_bullets_update(gs, dt);
    boss_update(gs, dt);
    gem_update(gs, dt);
    items_update(gs, dt);
    chests_update(gs, dt);
    particles_update(gs, dt);
}

static void draw_game_world(const GameState *gs) {
    Vector2 shake_offset = gs->offset;
    if (gs->shake_amount > 0) {
        float sx = ((rand() % 200) / 100.0f - 1.0f) * gs->shake_amount;
        float sy = ((rand() % 200) / 100.0f - 1.0f) * gs->shake_amount;
        shake_offset.x += sx;
        shake_offset.y += sy;
    }

    render_background();
    BeginBlendMode(BLEND_ADDITIVE);

    trail_draw(gs, gs->scale, shake_offset);
    mines_draw(gs, gs->scale, shake_offset);
    bullet_draw(gs->bullets, gs->scale, shake_offset);
    orbiters_draw(gs, gs->scale, shake_offset);
    beam_draw(gs, gs->scale, shake_offset);
    nova_draw(gs, gs->scale, shake_offset);
    chain_draw(gs, gs->scale, shake_offset);
    boomerang_draw(gs, gs->scale, shake_offset);
    whip_draw(gs, gs->scale, shake_offset);
    enemy_draw(gs->enemies, gs->scale, shake_offset);
    enemy_bullets_draw(gs, gs->scale, shake_offset);
    boss_draw(gs, gs->scale, shake_offset);
    gem_draw(gs->gems, gs->scale, shake_offset);
    items_draw(gs, gs->scale, shake_offset);
    chests_draw(gs, gs->scale, shake_offset);
    particles_draw(gs, gs->scale, shake_offset);
    player_draw(&gs->player, gs->scale, shake_offset);

    EndBlendMode();

    player_draw_hp_bar(&gs->player, gs->scale, shake_offset);
    popups_draw(gs, gs->scale, shake_offset);
}

static void write_stats(const GameState *gs, const char *path,
                        const ParamSet *p, bool ran_to_completion) {
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "duration=%.3f\n", gs->game_time);
    fprintf(f, "survived=%d\n", (gs->player.hp > 0 && !gs->game_over) ? 1
                                  : (gs->victory ? 1 : 0));
    fprintf(f, "ran_to_completion=%d\n", ran_to_completion ? 1 : 0);
    fprintf(f, "boss_defeated=%d\n", gs->boss_defeated ? 1 : 0);
    fprintf(f, "boss_active=%d\n", gs->boss.active ? 1 : 0);
    fprintf(f, "kills=%d\n", gs->kills);
    fprintf(f, "level=%d\n", gs->level);
    fprintf(f, "final_hp=%d\n", gs->player.hp);
    fprintf(f, "max_hp=%d\n", gs->player.max_hp);
    fprintf(f, "param_enemy_hp_mult=%f\n", p->enemy_hp_mult);
    fprintf(f, "param_enemy_spawn_min_mult=%f\n", p->enemy_spawn_min_mult);
    fprintf(f, "param_enemy_speed_bonus_mult=%f\n", p->enemy_speed_bonus_mult);
    fprintf(f, "param_enemy_damage_mult=%f\n", p->enemy_damage_mult);
    fprintf(f, "param_spawn_count_mult=%f\n", p->spawn_count_mult);
    fprintf(f, "param_player_speed_mult=%f\n", p->player_speed_mult);
    fprintf(f, "param_player_hp_mult=%f\n", p->player_hp_mult);
    fprintf(f, "param_player_invincible_mult=%f\n", p->player_invincible_mult);
    fclose(f);
}

static int run_headless(const CliOptions *opt) {
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(WINDOW_W, WINDOW_H, "DISK SURVIVOR (headless)");

    GameState gs = {0};
    gs.scene = SCENE_GAME;
    gs.bot_mode = true;
    gs.headless = true;
    score_load(&gs.best);
    game_init(&gs);
    gs.has_pulse_bolt = true;

    float dt = 0.033f;
    float max_dur = opt->duration_set ? opt->duration_override : GAME_DURATION;
    int max_ticks = (int)(max_dur / dt) + 100;
    int ticks = 0;
    bool ran_to_completion = false;

    while (gs.player.hp > 0 && gs.game_time < max_dur) {
        if (gs.upgrading) {
            upgrade_auto_pick(&gs);
        }
        if (ticks++ > max_ticks) break;

        gs.game_time += dt;
        if (!gs.boss.active && !gs.boss_defeated &&
            gs.game_time >= BOSS_SPAWN_TIME) {
            boss_spawn(&gs);
        }
        player_update(&gs, dt, 1.0f);
        weapon_update(&gs, dt);
        bullet_update(&gs, dt);
        orbiters_update(&gs, dt);
        beam_update(&gs, dt);
        nova_update(&gs, dt);
        mines_update(&gs, dt);
        chain_update(&gs, dt);
        boomerang_update(&gs, dt);
        trail_update(&gs, dt);
        whip_update(&gs, dt);
        enemy_update(&gs, dt);
        enemy_bullets_update(&gs, dt);
        boss_update(&gs, dt);
        gem_update(&gs, dt);
        items_update(&gs, dt);
        chests_update(&gs, dt);
        particles_update(&gs, dt);
    }
    ran_to_completion = (gs.game_time >= max_dur);

    if (opt->output_path) {
        write_stats(&gs, opt->output_path, &g_params, ran_to_completion);
    } else {
        fprintf(stderr, "duration=%.2f kills=%d level=%d boss=%d hp=%d/%d\n",
            gs.game_time, gs.kills, gs.level,
            gs.boss_defeated ? 1 : 0,
            gs.player.hp, gs.player.max_hp);
    }

    CloseWindow();
    return 0;
}

int main(int argc, char **argv) {
    CliOptions opt;
    cli_parse(argc, argv, &opt);
    params_from_difficulty(&g_params, (float)DIFFICULTY);
    if (opt.params_arg && !params_apply_kv(&g_params, opt.params_arg)) {
        return 1;
    }
    srand(opt.seed_set ? opt.seed : (unsigned)time(NULL));

    if (opt.headless) {
        return run_headless(&opt);
    }

    InitWindow(WINDOW_W, WINDOW_H, "DISK SURVIVOR");
    SetTargetFPS(TARGET_FPS);
    SetExitKey(KEY_NULL);
    audio_init();
    bgm_init();
    bgm_play(BGM_TITLE);

    RenderTexture2D target = LoadRenderTexture(WINDOW_W, WINDOW_H);
    Shader bloom = LoadShaderFromMemory(0, bloomShaderCode);

    int resLoc = GetShaderLocation(bloom, "resolution");
    float resolution[2] = {(float)WINDOW_W, (float)WINDOW_H};
    SetShaderValue(bloom, resLoc, resolution, SHADER_UNIFORM_VEC2);

    GameState gs = {0};
    gs.scene = SCENE_TITLE;
    gs.scene_timer = 0;
    gs.bot_mode = opt.bot;
    score_load(&gs.best);
    game_init(&gs);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > DT_MAX) dt = DT_MAX;

        switch (gs.scene) {
            case SCENE_TITLE:
                bgm_play(BGM_TITLE);
                scene_title_update(&gs, dt);
                break;
            case SCENE_WEAPON_SELECT:
                bgm_play(BGM_TITLE);
                scene_weapon_select_update(&gs, dt);
                break;
            case SCENE_GAME:
                bgm_play(gs.boss.active ? BGM_BOSS : BGM_GAME);
                update_game(&gs, dt);
                break;
            case SCENE_RESULT:
                bgm_play(BGM_TITLE);
                scene_result_update(&gs, dt);
                break;
        }

        BeginTextureMode(target);
        if (gs.scene == SCENE_TITLE || gs.scene == SCENE_WEAPON_SELECT) {
            render_background();
        } else {
            draw_game_world(&gs);
        }
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);

        BeginShaderMode(bloom);
        DrawTextureRec(target.texture,
            (Rectangle){0, 0, (float)target.texture.width, (float)-target.texture.height},
            (Vector2){0, 0}, WHITE);
        EndShaderMode();

        switch (gs.scene) {
            case SCENE_TITLE:
                scene_title_draw(&gs);
                break;
            case SCENE_WEAPON_SELECT:
                scene_weapon_select_draw(&gs);
                break;
            case SCENE_GAME:
                flash_draw(&gs);
                draw_hud(&gs);
                if (gs.upgrading) upgrade_draw(&gs);
                if (gs.paused) pause_draw(&gs);
                debug_draw(&gs);
                break;
            case SCENE_RESULT:
                draw_hud(&gs);
                scene_result_draw(&gs);
                break;
        }

        EndDrawing();
    }

    UnloadShader(bloom);
    UnloadRenderTexture(target);
    bgm_cleanup();
    audio_cleanup();
    CloseWindow();
    return 0;
}
