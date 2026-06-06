#include "game.h"
#include <math.h>
#include <stdlib.h>

void items_init(GameState *gs) {
    for (int i = 0; i < MAX_ITEMS; i++) gs->items[i].active = false;
    for (int i = 0; i < MAX_CHESTS; i++) gs->chests[i].active = false;
}

static void spawn_item(GameState *gs, Vector2 pos, ItemType type) {
    for (int i = 0; i < MAX_ITEMS; i++) {
        if (!gs->items[i].active) {
            gs->items[i].active = true;
            gs->items[i].pos = pos;
            gs->items[i].type = type;
            gs->items[i].life = ITEM_LIFE;
            return;
        }
    }
}

static void spawn_chest(GameState *gs, Vector2 pos) {
    for (int i = 0; i < MAX_CHESTS; i++) {
        if (!gs->chests[i].active) {
            gs->chests[i].active = true;
            gs->chests[i].pos = pos;
            gs->chests[i].life = CHEST_LIFE;
            return;
        }
    }
}

// 1 = item drop candidate (medium/large regular enemies)
static int drop_tier(EnemyType type) {
    switch (type) {
        case ENEMY_GLITCH:
        case ENEMY_BOMBER:
        case ENEMY_RANGER:
        case ENEMY_PHASER:
        case ENEMY_TRACKER:
        case ENEMY_SPLITTER:
        case ENEMY_PACKET:
        case ENEMY_BADSECTOR:
            return 1;
        default:
            return 0;
    }
}

void item_drop_roll(GameState *gs, EnemyType type, Vector2 pos) {
    if (drop_tier(type) == 0) return;
    int roll = rand() % 100;
    if (roll < ITEM_DROP_CHANCE) {
        ItemType it = (rand() % 2 == 0) ? ITEM_HP : ITEM_MAGNET;
        spawn_item(gs, pos, it);
    }
}

void chest_drop(GameState *gs, Vector2 pos) {
    spawn_chest(gs, pos);
}

static void apply_item(GameState *gs, ItemType type) {
    switch (type) {
        case ITEM_HP:
            gs->player.hp += ITEM_HP_HEAL;
            if (gs->player.hp > gs->player.max_hp) gs->player.hp = gs->player.max_hp;
            popup_spawn(gs, gs->player.pos, ITEM_HP_HEAL, (Color){120, 255, 120, 255});
            particles_spawn_burst(gs, gs->player.pos, (Color){120, 255, 120, 255}, 18);
            flash_trigger(gs, (Color){120, 255, 120, 255}, 0.35f);
            break;
        case ITEM_MAGNET:
            gs->magnet_pull_timer = 2.0f;
            particles_spawn_burst(gs, gs->player.pos, (Color){150, 255, 150, 255}, 18);
            flash_trigger(gs, (Color){150, 255, 200, 255}, 0.3f);
            break;
        default:
            break;
    }
    audio_play(SFX_LEVEL_UP);
}

void items_update(GameState *gs, float dt) {
    for (int i = 0; i < MAX_ITEMS; i++) {
        if (!gs->items[i].active) continue;
        Item *it = &gs->items[i];
        it->life -= dt;
        if (it->life <= 0) {
            it->active = false;
            continue;
        }
        float dx = gs->player.pos.x - it->pos.x;
        float dy = gs->player.pos.y - it->pos.y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist < PLAYER_RADIUS + ITEM_RADIUS) {
            apply_item(gs, it->type);
            it->active = false;
        }
    }
}

void chests_update(GameState *gs, float dt) {
    for (int i = 0; i < MAX_CHESTS; i++) {
        if (!gs->chests[i].active) continue;
        Chest *c = &gs->chests[i];
        c->life -= dt;
        if (c->life <= 0) {
            c->active = false;
            continue;
        }
        float dx = gs->player.pos.x - c->pos.x;
        float dy = gs->player.pos.y - c->pos.y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist < PLAYER_RADIUS + CHEST_RADIUS) {
            c->active = false;
            particles_spawn_burst(gs, c->pos, (Color){150, 230, 255, 255}, 40);
            flash_trigger(gs, (Color){150, 230, 255, 255}, 0.6f);
            shake_add(gs, SHAKE_HIT);
            audio_play(SFX_LEVEL_UP);
            upgrade_start(gs);
        }
    }
}

static void draw_blink(float life, float max_life, float *alpha_out) {
    *alpha_out = 1.0f;
    if (life < max_life * 0.3f) {
        float blink = sinf(life * 14.0f);
        if (blink < 0) *alpha_out = 0.4f;
    }
}

void items_draw(const GameState *gs, float scale, Vector2 offset) {
    float t = (float)GetTime();
    for (int i = 0; i < MAX_ITEMS; i++) {
        if (!gs->items[i].active) continue;
        const Item *it = &gs->items[i];
        float bob = sinf(t * 3.0f + i) * 3.0f;
        float x = it->pos.x * scale + offset.x;
        float y = (it->pos.y + bob) * scale + offset.y;
        float r = ITEM_RADIUS * scale;
        float a;
        draw_blink(it->life, ITEM_LIFE, &a);

        if (it->type == ITEM_HP) {
            Color outer = {120, 255, 120, (unsigned char)(255 * a)};
            Color inner = {220, 255, 220, (unsigned char)(255 * a)};
            DrawCircleV((Vector2){x, y}, r * 1.2f,
                (Color){120, 255, 120, (unsigned char)(80 * a)});
            DrawRectangle((int)(x - r * 0.25f), (int)(y - r * 0.8f),
                (int)(r * 0.5f), (int)(r * 1.6f), outer);
            DrawRectangle((int)(x - r * 0.8f), (int)(y - r * 0.25f),
                (int)(r * 1.6f), (int)(r * 0.5f), outer);
            DrawRectangle((int)(x - r * 0.15f), (int)(y - r * 0.7f),
                (int)(r * 0.3f), (int)(r * 1.4f), inner);
            DrawRectangle((int)(x - r * 0.7f), (int)(y - r * 0.15f),
                (int)(r * 1.4f), (int)(r * 0.3f), inner);
        } else {
            // Magnet: bigger glowing gem (diamond) like a super-gem
            float pulse = 1.0f + 0.15f * sinf(t * 4.0f + i * 0.3f);
            float rr = r * 1.4f * pulse;
            Color outer = {120, 255, 120, (unsigned char)(255 * a)};
            Color inner = {220, 255, 220, (unsigned char)(255 * a)};
            Color glow  = {120, 255, 120, (unsigned char)(90 * a)};

            DrawCircleV((Vector2){x, y}, rr * 1.4f, glow);

            Vector2 top    = {x, y - rr * 1.2f};
            Vector2 right  = {x + rr * 0.9f, y};
            Vector2 bottom = {x, y + rr * 1.2f};
            Vector2 left   = {x - rr * 0.9f, y};
            DrawTriangle(top, left, right, outer);
            DrawTriangle(bottom, right, left, outer);

            Vector2 itop    = {x, y - rr * 0.55f};
            Vector2 iright  = {x + rr * 0.45f, y};
            Vector2 ibottom = {x, y + rr * 0.55f};
            Vector2 ileft   = {x - rr * 0.45f, y};
            DrawTriangle(itop, ileft, iright, inner);
            DrawTriangle(ibottom, iright, ileft, inner);

            // Orbiting sparkle gems hinting at "magnet" effect
            for (int k = 0; k < 3; k++) {
                float ang = t * 3.0f + k * 2.094f;
                float sx = x + cosf(ang) * rr * 1.6f;
                float sy = y + sinf(ang) * rr * 1.6f;
                float sr = rr * 0.25f;
                Vector2 stop_   = {sx, sy - sr};
                Vector2 sright  = {sx + sr * 0.75f, sy};
                Vector2 sbot    = {sx, sy + sr};
                Vector2 sleft   = {sx - sr * 0.75f, sy};
                DrawTriangle(stop_, sleft, sright, outer);
                DrawTriangle(sbot, sright, sleft, outer);
            }
        }
    }
}

void chests_draw(const GameState *gs, float scale, Vector2 offset) {
    float t = (float)GetTime();
    for (int i = 0; i < MAX_CHESTS; i++) {
        if (!gs->chests[i].active) continue;
        const Chest *c = &gs->chests[i];
        float bob = sinf(t * 2.5f + i) * 3.0f;
        float x = c->pos.x * scale + offset.x;
        float y = (c->pos.y + bob) * scale + offset.y;
        float r = CHEST_RADIUS * scale;
        float a;
        draw_blink(c->life, CHEST_LIFE, &a);

        Color outer = {100, 220, 255, (unsigned char)(255 * a)};
        Color inner = {220, 245, 255, (unsigned char)(255 * a)};
        Color glow  = {100, 220, 255, (unsigned char)(80 * a)};

        float pulse = 1.0f + 0.1f * sinf(t * 4.0f);
        DrawCircleV((Vector2){x, y}, r * 1.5f * pulse, glow);
        DrawRectangle((int)(x - r), (int)(y - r * 0.7f),
            (int)(r * 2), (int)(r * 1.4f), outer);
        DrawRectangle((int)(x - r * 0.85f), (int)(y - r * 0.55f),
            (int)(r * 1.7f), (int)(r * 1.1f), inner);
        DrawRectangle((int)(x - r * 0.15f), (int)(y - r * 0.7f),
            (int)(r * 0.3f), (int)(r * 1.4f), outer);
        // Glints
        DrawCircleV((Vector2){x - r * 0.4f, y - r * 0.3f}, r * 0.1f,
            (Color){255, 255, 255, (unsigned char)(255 * a)});
        DrawCircleV((Vector2){x + r * 0.5f, y + r * 0.2f}, r * 0.08f,
            (Color){255, 255, 255, (unsigned char)(255 * a)});
    }
}
