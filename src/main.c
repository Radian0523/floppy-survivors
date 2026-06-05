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

    gs->xp = 0;
    gs->level = 1;
    gs->xp_to_next = XP_BASE_REQUIREMENT;

    gs->fire_timer = 0;
    gs->fire_interval = WEAPON_FIRE_INTERVAL;
    gs->bullet_count = 1;
    gs->bullet_damage = BULLET_DAMAGE;

    gs->has_orbiters = false;
    gs->orbiter_count = 0;
    gs->has_beam = false;
    gs->has_nova = false;

    gs->spawn_timer = 1.0f;
    gs->spawn_interval = SPAWN_INTERVAL_INITIAL;

    gs->game_time = 0;
    gs->game_over = false;
    gs->victory = false;
    gs->kills = 0;

    gs->boss.active = false;
    gs->boss_defeated = false;

    gs->upgrading = false;
    gs->upgrade_hover = -1;
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
    if (gs->upgrading) {
        upgrade_update(gs);
        return;
    }

    gs->game_time += dt;

    if (!gs->boss.active && !gs->boss_defeated && gs->game_time >= BOSS_SPAWN_TIME) {
        boss_spawn(gs);
    }

    if (gs->boss_defeated) {
        gs->game_over = true;
        gs->victory = true;
        gs->scene = SCENE_RESULT;
        gs->scene_timer = 0;
        return;
    }

    if (gs->game_time >= GAME_DURATION) {
        gs->game_over = true;
        gs->victory = true;
        gs->scene = SCENE_RESULT;
        gs->scene_timer = 0;
        return;
    }

    if (gs->player.hp <= 0) {
        gs->game_over = true;
        gs->victory = false;
        gs->scene = SCENE_RESULT;
        gs->scene_timer = 0;
        return;
    }

    player_update(&gs->player, dt, gs->scale);
    weapon_update(gs, dt);
    bullet_update(gs, dt);
    orbiters_update(gs, dt);
    beam_update(gs, dt);
    nova_update(gs, dt);
    enemy_update(gs, dt);
    boss_update(gs, dt);
    gem_update(gs, dt);
}

static void draw_game_world(const GameState *gs) {
    render_background();
    BeginBlendMode(BLEND_ADDITIVE);

    bullet_draw(gs->bullets, gs->scale, gs->offset);
    orbiters_draw(gs, gs->scale, gs->offset);
    beam_draw(gs, gs->scale, gs->offset);
    nova_draw(gs, gs->scale, gs->offset);
    enemy_draw(gs->enemies, gs->scale, gs->offset);
    boss_draw(gs, gs->scale, gs->offset);
    gem_draw(gs->gems, gs->scale, gs->offset);
    player_draw(&gs->player, gs->scale, gs->offset);

    EndBlendMode();
}

int main(void) {
    srand((unsigned)time(NULL));

    InitWindow(WINDOW_W, WINDOW_H, "DISK SURVIVOR");
    SetTargetFPS(TARGET_FPS);
    audio_init();

    RenderTexture2D target = LoadRenderTexture(WINDOW_W, WINDOW_H);
    Shader bloom = LoadShaderFromMemory(0, bloomShaderCode);

    int resLoc = GetShaderLocation(bloom, "resolution");
    float resolution[2] = {(float)WINDOW_W, (float)WINDOW_H};
    SetShaderValue(bloom, resLoc, resolution, SHADER_UNIFORM_VEC2);

    GameState gs = {0};
    gs.scene = SCENE_TITLE;
    gs.scene_timer = 0;
    game_init(&gs);

    GameScene prev_scene = SCENE_TITLE;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > DT_MAX) dt = DT_MAX;

        switch (gs.scene) {
            case SCENE_TITLE:
                if (prev_scene != SCENE_TITLE) {
                    game_init(&gs);
                    gs.scene = SCENE_TITLE;
                    gs.scene_timer = 0;
                }
                scene_title_update(&gs, dt);
                if (gs.scene == SCENE_GAME) {
                    game_init(&gs);
                    gs.scene = SCENE_GAME;
                }
                break;
            case SCENE_GAME:
                update_game(&gs, dt);
                break;
            case SCENE_RESULT:
                scene_result_update(&gs, dt);
                break;
        }
        prev_scene = gs.scene;

        BeginTextureMode(target);
        if (gs.scene == SCENE_TITLE) {
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
            case SCENE_GAME:
                draw_hud(&gs);
                if (gs.upgrading) upgrade_draw(&gs);
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
    audio_cleanup();
    CloseWindow();
    return 0;
}
