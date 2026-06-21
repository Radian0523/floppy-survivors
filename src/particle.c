#include "game.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

void particles_init(GameState *gs) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        gs->particles[i].active = false;
    }
    for (int i = 0; i < MAX_POPUPS; i++) {
        gs->popups[i].active = false;
    }
    gs->shake_amount = 0;
    gs->flash_amount = 0;
}

// Dynamic decimation thresholds (combat against overdraw + visual chaos).
// Once active particle count crosses these thresholds, new bursts spawn fewer
// particles and existing tails fade faster. Ref: Christer Ericson on additive
// particle overdraw cost.
#define PARTICLE_SOFT_LIMIT (MAX_PARTICLES * 3 / 4)   // 75%: half-count bursts
#define PARTICLE_HARD_LIMIT (MAX_PARTICLES * 9 / 10)  // 90%: drop new bursts

static int count_active_particles(const GameState *gs) {
    int n = 0;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (gs->particles[i].active) n++;
    }
    return n;
}

void particles_spawn_burst(GameState *gs, Vector2 pos, Color color, int count) {
    // Pressure-based decimation: skip or shrink new bursts if particles
    // are already crowded. Critical bursts (kills, hits) should still register
    // visually, but extras (sparkles, decorations) bow out.
    int active = count_active_particles(gs);
    if (active >= PARTICLE_HARD_LIMIT) {
        return;  // Drop entirely; the screen is already overloaded
    }
    if (active >= PARTICLE_SOFT_LIMIT) {
        count = count / 2;
        if (count < 1) count = 1;
    }

    int spawned = 0;
    for (int i = 0; i < MAX_PARTICLES && spawned < count; i++) {
        if (gs->particles[i].active) continue;

        Particle *p = &gs->particles[i];
        p->active = true;
        p->pos = pos;

        float angle = ((rand() % 360) / 180.0f) * 3.14159f;
        float speed = PARTICLE_SPEED * (0.5f + (rand() % 100) / 200.0f);
        p->vel.x = cosf(angle) * speed;
        p->vel.y = sinf(angle) * speed;

        float life_var = (rand() % 100) / 200.0f;
        p->life = PARTICLE_LIFE * (0.6f + life_var);
        // Under soft pressure, shorten lifetime so the pool churns faster.
        if (active >= PARTICLE_SOFT_LIMIT) p->life *= 0.6f;
        p->max_life = p->life;
        p->color = color;

        spawned++;
    }
}

void particles_update(GameState *gs, float dt) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!gs->particles[i].active) continue;

        Particle *p = &gs->particles[i];
        p->pos.x += p->vel.x * dt;
        p->pos.y += p->vel.y * dt;
        p->vel.x *= 0.92f;
        p->vel.y *= 0.92f;
        p->life -= dt;
        if (p->life <= 0) {
            p->active = false;
        }
    }

    if (gs->shake_amount > 0) {
        gs->shake_amount -= SHAKE_DECAY * dt;
        if (gs->shake_amount < 0) gs->shake_amount = 0;
    }

    if (gs->flash_amount > 0) {
        gs->flash_amount -= FLASH_DECAY * dt;
        if (gs->flash_amount < 0) gs->flash_amount = 0;
    }

    popups_update(gs, dt);
}

void particles_draw(const GameState *gs, float scale, Vector2 offset) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!gs->particles[i].active) continue;

        const Particle *p = &gs->particles[i];
        float ratio = p->life / p->max_life;
        float x = p->pos.x * scale + offset.x;
        float y = p->pos.y * scale + offset.y;
        float r = 3.0f * scale * ratio;

        Color c = p->color;
        c.a = (unsigned char)(c.a * ratio);

        DrawCircleV((Vector2){x, y}, r, c);
    }
}

void shake_add(GameState *gs, float amount) {
    if (amount > gs->shake_amount) {
        gs->shake_amount = amount;
    }
}

void popup_spawn(GameState *gs, Vector2 pos, int value, Color color) {
    for (int i = 0; i < MAX_POPUPS; i++) {
        if (!gs->popups[i].active) {
            gs->popups[i].active = true;
            gs->popups[i].pos = pos;
            gs->popups[i].pos.x += ((rand() % 20) - 10);
            gs->popups[i].pos.y += ((rand() % 10) - 5);
            gs->popups[i].life = POPUP_LIFE;
            gs->popups[i].max_life = POPUP_LIFE;
            gs->popups[i].value = value;
            gs->popups[i].color = color;
            return;
        }
    }
}

void popups_update(GameState *gs, float dt) {
    for (int i = 0; i < MAX_POPUPS; i++) {
        if (!gs->popups[i].active) continue;
        Popup *p = &gs->popups[i];
        p->pos.y -= POPUP_RISE_SPEED * dt;
        p->life -= dt;
        if (p->life <= 0) p->active = false;
    }
}

void popups_draw(const GameState *gs, float scale, Vector2 offset) {
    char buf[16];
    for (int i = 0; i < MAX_POPUPS; i++) {
        if (!gs->popups[i].active) continue;
        const Popup *p = &gs->popups[i];
        float ratio = p->life / p->max_life;
        float x = p->pos.x * scale + offset.x;
        float y = p->pos.y * scale + offset.y;
        sprintf(buf, "%d", p->value);
        int size = 14;
        int tw = MeasureText(buf, size);
        Color c = p->color;
        c.a = (unsigned char)(255 * ratio);
        Color shadow = {0, 0, 0, (unsigned char)(180 * ratio)};
        DrawText(buf, (int)(x - tw / 2 + 1), (int)(y + 1), size, shadow);
        DrawText(buf, (int)(x - tw / 2), (int)y, size, c);
    }
}

void flash_trigger(GameState *gs, Color color, float amount) {
    if (amount > gs->flash_amount) {
        gs->flash_amount = amount;
        gs->flash_color = color;
    }
}

void flash_draw(const GameState *gs) {
    if (gs->flash_amount <= 0) return;
    Color c = gs->flash_color;
    c.a = (unsigned char)(255 * gs->flash_amount);
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), c);
}
