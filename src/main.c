#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

    gs->weapon_rate_mult = 1.0f;
    gs->weapon_damage_bonus = 0;
    gs->weapon_extra_projectiles = 0;
    gs->weapon_area_mult = 1.0f;
    gs->weapon_duration_mult = 1.0f;

    gs->pulse.has = false;
    gs->pulse.fire_timer = 0;
    gs->pulse.fire_interval = WEAPON_FIRE_INTERVAL;
    gs->pulse.bullet_count = 1;
    gs->pulse.damage = BULLET_DAMAGE;

    gs->orbiters.has = false;
    gs->orbiters.count = 0;
    gs->beam.has = false;
    gs->nova.has = false;
    gs->mines.has = false;
    gs->chain.has = false;
    gs->boomerang.has = false;
    gs->trail.has = false;
    gs->whip.has = false;

    gs->spawn_timer = 1.0f;
    gs->spawn_interval = SPAWN_INTERVAL_INITIAL;
    gs->elite_timer = ELITE_FIRST_TIME;
    gs->formation_timer = FORMATION_FIRST_TIME;

    gs->game_time = 0;
    gs->game_over = false;
    gs->victory = false;
    gs->kills = 0;
    memset(&gs->stats, 0, sizeof(gs->stats));

    gs->boss.active = false;
    gs->boss_defeated = false;

    particles_init(gs);
    items_init(gs);
    gs->magnet_pull_timer = 0;

    gs->upgrading = false;
    gs->upgrade_hover = -1;
    gs->paused = false;
    gs->game_speed = 1;
    gs->last_score_flags = 0;
    for (int i = 0; i < UPGRADE_COUNT; i++) gs->upgrade_picks[i] = 0;
}

void game_start_with_weapon(GameState *gs, StartingWeapon weapon) {
    game_init(gs);
    switch (weapon) {
        case STARTING_WEAPON_PULSE:
            gs->pulse.has = true;
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
            gs->pulse.has = true;
            break;
    }
}

static void draw_hud(const GameState *gs) {
    char buf[64];
    int sw = GetScreenWidth();

    // === Full-width XP bar (top edge) ===
    int xp_bar_h = 14;
    int xp_bar_y = 0;
    float xp_ratio = (gs->xp_to_next > 0)
        ? (float)gs->xp / gs->xp_to_next : 0;
    if (xp_ratio > 1.0f) xp_ratio = 1.0f;
    // Base track
    DrawRectangle(0, xp_bar_y, sw, xp_bar_h, (Color){12, 18, 28, 230});
    // Decile tick marks
    for (int i = 1; i < 10; i++) {
        int tx = sw * i / 10;
        DrawRectangle(tx, xp_bar_y + xp_bar_h - 3, 1, 3,
                      (Color){60, 80, 100, 200});
    }
    // Fill
    int fill_w = (int)(sw * xp_ratio);
    if (fill_w > 0) {
        DrawRectangle(0, xp_bar_y, fill_w, xp_bar_h,
                      (Color){80, 220, 130, 230});
        // Leading edge glow
        if (fill_w < sw) {
            DrawRectangle(fill_w - 2, xp_bar_y, 4, xp_bar_h,
                          (Color){200, 255, 200, 200});
        }
    }
    // Bottom border line
    DrawRectangle(0, xp_bar_y + xp_bar_h, sw, 1,
                  (Color){60, 100, 120, 200});

    // LV badge overlay (left side of the bar)
    sprintf(buf, "LV %d", gs->level);
    int lv_w = MeasureText(buf, 12);
    int lv_pad_x = 8;
    int lv_box_w = lv_w + lv_pad_x * 2;
    DrawRectangle(0, xp_bar_y, lv_box_w, xp_bar_h,
                  (Color){10, 14, 24, 200});
    DrawText(buf, lv_pad_x, xp_bar_y + 1, 12, (Color){180, 255, 255, 255});

    // XP fraction on right side of the bar (only when not near zero)
    sprintf(buf, "%d / %d", gs->xp, gs->xp_to_next);
    int xp_text_w = MeasureText(buf, 10);
    DrawText(buf, sw - xp_text_w - 6, xp_bar_y + 2, 10,
             (Color){240, 240, 240, 220});

    // === Below the XP bar: standard HUD ===
    int hud_y = xp_bar_h + 8;

    int minutes = (int)gs->game_time / 60;
    int seconds = (int)gs->game_time % 60;
    sprintf(buf, "TIME %d:%02d", minutes, seconds);
    DrawText(buf, 10, hud_y, 20, (Color){200, 200, 200, 255});

    sprintf(buf, "HP %d/%d", gs->player.hp, gs->player.max_hp);
    DrawText(buf, 10, hud_y + 25, 20, (Color){255, 100, 100, 255});

    // Speed control (replaces KILLS at top-right): current speed + key hint
    sprintf(buf, "SPEED x%d", gs->game_speed);
    Color spd_col = (gs->game_speed > 1)
        ? (Color){255, 220, 120, 255}
        : (Color){200, 200, 200, 255};
    int sw_text = MeasureText(buf, 20);
    DrawText(buf, sw - sw_text - 10, hud_y, 20, spd_col);

    const char *hint = "Z: cycle 1x / 2x / 3x";
    int hw = MeasureText(hint, 12);
    DrawText(hint, sw - hw - 10, hud_y + 23, 12, (Color){140, 140, 160, 200});
}

static void update_game_step(GameState *gs, float dt);

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

    // Z cycles game speed: 1x -> 2x -> 3x -> 1x
    if (IsKeyPressed(KEY_Z)) {
        gs->game_speed = (gs->game_speed % 3) + 1;
    }

    // Run N independent sub-steps so collision/spawn timing stays consistent
    // (rather than multiplying dt, which would let fast bullets tunnel).
    int steps = gs->game_speed;
    if (steps < 1) steps = 1;
    for (int s = 0; s < steps; s++) {
        update_game_step(gs, dt);
        if (gs->scene != SCENE_GAME || gs->upgrading) break;
    }
}

static void update_game_step(GameState *gs, float dt) {
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
    fprintf(f, "damage_taken=%d\n", gs->stats.damage_taken);
    fprintf(f, "gems_collected=%d\n", gs->stats.gems_collected);
    fprintf(f, "items_collected=%d\n", gs->stats.items_collected);
    fprintf(f, "xp_collected=%d\n", gs->stats.xp_collected);
    for (int w = 0; w < WEAPON_ID_COUNT; w++) {
        fprintf(f, "wpn_dmg_%d=%d\n", w, gs->stats.damage_dealt[w]);
        fprintf(f, "wpn_kill_%d=%d\n", w, gs->stats.kills_by[w]);
    }
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
    gs.pulse.has = true;
    if (opt->all_weapons) {
        orbiters_init(&gs);
        beam_init(&gs);
        nova_init(&gs);
        mines_init(&gs);
        chain_init(&gs);
        boomerang_init(&gs);
        trail_init(&gs);
        whip_init(&gs);
    }

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
    gs.difficulty = DIFFICULTY;
    settings_load(&gs.settings);
    settings_apply(&gs.settings);
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
            case SCENE_HOW_TO_PLAY:
                bgm_play(BGM_TITLE);
                scene_how_to_play_update(&gs, dt);
                break;
            case SCENE_SETTINGS:
                bgm_play(BGM_TITLE);
                scene_settings_update(&gs, dt);
                break;
            case SCENE_WEAPON_SELECT:
                bgm_play(BGM_TITLE);
                scene_weapon_select_update(&gs, dt);
                break;
            case SCENE_GAME: {
                BgmId track = BGM_GAME;
                if (gs.boss.active) track = BGM_BOSS;
                else if (gs.game_time >= 180.0f) track = BGM_GAME_LATE;
                bgm_play(track);
                update_game(&gs, dt);
                break;
            }
            case SCENE_RESULT:
                bgm_play(BGM_TITLE);
                scene_result_update(&gs, dt);
                break;
        }

        BeginTextureMode(target);
        if (gs.scene == SCENE_TITLE || gs.scene == SCENE_WEAPON_SELECT ||
            gs.scene == SCENE_HOW_TO_PLAY || gs.scene == SCENE_SETTINGS) {
            render_background();
            if (gs.scene == SCENE_TITLE || gs.scene == SCENE_HOW_TO_PLAY ||
                gs.scene == SCENE_SETTINGS) {
                BeginBlendMode(BLEND_ADDITIVE);
                scene_title_draw_world();
                EndBlendMode();
            }
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
            case SCENE_HOW_TO_PLAY:
                scene_how_to_play_draw(&gs);
                break;
            case SCENE_SETTINGS:
                scene_settings_draw(&gs);
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
