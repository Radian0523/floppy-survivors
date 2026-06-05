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

int main(void) {
    srand((unsigned)time(NULL));

    InitWindow(WINDOW_W, WINDOW_H, "DISK SURVIVOR");
    SetTargetFPS(TARGET_FPS);

    RenderTexture2D target = LoadRenderTexture(WINDOW_W, WINDOW_H);
    Shader bloom = LoadShaderFromMemory(0, bloomShaderCode);

    int resLoc = GetShaderLocation(bloom, "resolution");
    float resolution[2] = {(float)WINDOW_W, (float)WINDOW_H};
    SetShaderValue(bloom, resLoc, resolution, SHADER_UNIFORM_VEC2);

    GameState gs = {0};
    game_init(&gs);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > DT_MAX) dt = DT_MAX;

        if (!gs.game_over) {
            if (gs.upgrading) {
                upgrade_update(&gs);
            } else {
                gs.game_time += dt;

                if (gs.game_time >= GAME_DURATION) {
                    gs.game_over = true;
                    gs.victory = true;
                }

                if (gs.player.hp <= 0) {
                    gs.game_over = true;
                    gs.victory = false;
                }

                player_update(&gs.player, dt, gs.scale);
                weapon_update(&gs, dt);
                bullet_update(&gs, dt);
                orbiters_update(&gs, dt);
                beam_update(&gs, dt);
                nova_update(&gs, dt);
                enemy_update(&gs, dt);
                gem_update(&gs, dt);
            }
        } else {
            if (IsKeyPressed(KEY_R)) {
                game_init(&gs);
            }
        }

        BeginTextureMode(target);
        render_background();
        BeginBlendMode(BLEND_ADDITIVE);

        bullet_draw(gs.bullets, gs.scale, gs.offset);
        orbiters_draw(&gs, gs.scale, gs.offset);
        beam_draw(&gs, gs.scale, gs.offset);
        nova_draw(&gs, gs.scale, gs.offset);
        enemy_draw(gs.enemies, gs.scale, gs.offset);
        gem_draw(gs.gems, gs.scale, gs.offset);
        player_draw(&gs.player, gs.scale, gs.offset);

        EndBlendMode();
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);

        BeginShaderMode(bloom);
        DrawTextureRec(target.texture,
            (Rectangle){0, 0, (float)target.texture.width, (float)-target.texture.height},
            (Vector2){0, 0}, WHITE);
        EndShaderMode();

        draw_hud(&gs);

        if (gs.upgrading) {
            upgrade_draw(&gs);
        }

        if (gs.game_over) {
            const char *msg = gs.victory ? "DISK RECOVERED!" : "DATA CORRUPTED";
            int w = MeasureText(msg, 40);
            DrawText(msg, (GetScreenWidth() - w) / 2, GetScreenHeight() / 2 - 40, 40,
                gs.victory ? (Color){100, 255, 100, 255} : (Color){255, 100, 100, 255});

            const char *hint = "Press R to restart";
            int hw = MeasureText(hint, 20);
            DrawText(hint, (GetScreenWidth() - hw) / 2, GetScreenHeight() / 2 + 20, 20,
                (Color){200, 200, 200, 255});
        }

        EndDrawing();
    }

    UnloadShader(bloom);
    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}
