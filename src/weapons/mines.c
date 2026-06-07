#include "../game.h"
#include "../weapon_util.h"
#include <math.h>

void mines_init(GameState *gs) {
    gs->mines.has = true;
    gs->mines.timer = MINE_INTERVAL;
    gs->mines.interval = MINE_INTERVAL;
    gs->mines.damage = MINE_DAMAGE;
    gs->mines.explosion_radius = MINE_EXPLOSION_RADIUS;
    for (int i = 0; i < MAX_MINES; i++) gs->mines.slots[i].active = false;
}

static void deploy_mine(GameState *gs, Vector2 pos) {
    for (int i = 0; i < MAX_MINES; i++) {
        if (!gs->mines.slots[i].active) {
            gs->mines.slots[i].active = true;
            gs->mines.slots[i].pos = pos;
            gs->mines.slots[i].life = MINE_LIFE * gs->weapon_duration_mult;
            return;
        }
    }
}

void mines_update(GameState *gs, float dt) {
    if (!gs->mines.has) return;

    gs->mines.timer -= dt;
    if (gs->mines.timer <= 0) {
        gs->mines.timer = gs->mines.interval * gs->weapon_rate_mult;
        deploy_mine(gs, gs->player.pos);
    }

    for (int i = 0; i < MAX_MINES; i++) {
        if (!gs->mines.slots[i].active) continue;
        Mine *m = &gs->mines.slots[i];
        m->life -= dt;

        bool detonate = (m->life <= 0);

        float trig_r = MINE_RADIUS * gs->weapon_area_mult;
        for (int j = 0; j < MAX_ENEMIES && !detonate; j++) {
            if (!gs->enemies[j].active) continue;
            if (gs->enemies[j].phased) continue;
            float dx = gs->enemies[j].pos.x - m->pos.x;
            float dy = gs->enemies[j].pos.y - m->pos.y;
            if (dx * dx + dy * dy < (trig_r + gs->enemies[j].radius) *
                                    (trig_r + gs->enemies[j].radius)) {
                detonate = true;
            }
        }
        if (!detonate && gs->boss.active) {
            float dx = gs->boss.pos.x - m->pos.x;
            float dy = gs->boss.pos.y - m->pos.y;
            if (dx * dx + dy * dy < (trig_r + BOSS_RADIUS) *
                                    (trig_r + BOSS_RADIUS)) {
                detonate = true;
            }
        }

        if (detonate) {
            weapon_aoe_damage(gs, m->pos,
                gs->mines.explosion_radius * gs->weapon_area_mult,
                gs->mines.damage,
                (Color){255, 220, 100, 255});
            m->active = false;
        }
    }
}

void mines_draw(const GameState *gs, float scale, Vector2 offset) {
    if (!gs->mines.has) return;
    float t = (float)GetTime();
    for (int i = 0; i < MAX_MINES; i++) {
        if (!gs->mines.slots[i].active) continue;
        float x = gs->mines.slots[i].pos.x * scale + offset.x;
        float y = gs->mines.slots[i].pos.y * scale + offset.y;
        float r = MINE_RADIUS * scale;
        float pulse = 0.5f + 0.5f * sinf(t * 8.0f);
        Color outer = {255, 220, 100, (unsigned char)(150 + 100 * pulse)};
        Color inner = {255, 255, 200, 255};
        DrawCircleV((Vector2){x, y}, r, outer);
        DrawCircleV((Vector2){x, y}, r * 0.5f, inner);
        DrawLineEx((Vector2){x - r, y - r}, (Vector2){x + r, y + r}, scale, outer);
        DrawLineEx((Vector2){x + r, y - r}, (Vector2){x - r, y + r}, scale, outer);
    }
}
