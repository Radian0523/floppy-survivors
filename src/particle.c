#include "game.h"
#include <math.h>
#include <stdlib.h>

void particles_init(GameState *gs) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        gs->particles[i].active = false;
    }
    gs->shake_amount = 0;
}

void particles_spawn_burst(GameState *gs, Vector2 pos, Color color, int count) {
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
