#include "game.h"
#include <math.h>

void render_background(void) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    // Recompute play-area transform (logical 640x400 centered in window).
    float sx = (float)sw / LOGICAL_W;
    float sy = (float)sh / LOGICAL_H;
    float scale = (sx < sy) ? sx : sy;
    float ox = (sw - LOGICAL_W * scale) * 0.5f;
    float oy = (sh - LOGICAL_H * scale) * 0.5f;
    float pw = LOGICAL_W * scale;
    float ph = LOGICAL_H * scale;
    float cx = ox + pw * 0.5f;
    float cy = oy + ph * 0.5f;

    ClearBackground((Color){4, 5, 12, 255});

    float t = (float)GetTime();

    // --- Disk theme: concentric circles (slow pulse) ---
    {
        float base = (LOGICAL_W < LOGICAL_H ? LOGICAL_W : LOGICAL_H) * scale * 0.08f;
        for (int i = 1; i <= 6; i++) {
            float r = base * i + sinf(t * 0.4f + i) * 2.0f;
            unsigned char a = 26 + (unsigned char)(8 * sinf(t * 0.7f + i));
            DrawCircleLines((int)cx, (int)cy, r, (Color){50, 90, 140, a});
        }
    }

    // --- Disk theme: radial sectors (slow rotation) ---
    {
        const int sectors = 16;
        float rot = t * 0.05f;
        float inner_r = 28.0f * scale;
        float outer_r = sqrtf(pw * pw + ph * ph) * 0.55f;
        for (int i = 0; i < sectors; i++) {
            float a = (2.0f * 3.14159f * i / sectors) + rot;
            Vector2 p1 = {cx + cosf(a) * inner_r, cy + sinf(a) * inner_r};
            Vector2 p2 = {cx + cosf(a) * outer_r, cy + sinf(a) * outer_r};
            DrawLineEx(p1, p2, 1.0f, (Color){40, 70, 110, 24});
        }
    }

    // --- Background grid with traveling pulse ---
    {
        int spacing = (int)(40.0f * scale);
        if (spacing < 12) spacing = 12;
        float wave = fmodf(t * 60.0f, (float)(sw + 200)) - 100.0f;

        Color grid_dim = {22, 32, 56, 80};
        Color grid_lit = {120, 200, 255, 200};

        int gx0 = ((int)ox) % spacing;
        for (int x = gx0; x < sw; x += spacing) {
            float dist = fabsf((float)x - wave);
            if (dist < 60.0f) {
                float k = (60.0f - dist) / 60.0f;
                Color c = {
                    (unsigned char)(grid_dim.r + (grid_lit.r - grid_dim.r) * k * 0.5f),
                    (unsigned char)(grid_dim.g + (grid_lit.g - grid_dim.g) * k * 0.5f),
                    (unsigned char)(grid_dim.b + (grid_lit.b - grid_dim.b) * k * 0.5f),
                    (unsigned char)(grid_dim.a + (grid_lit.a - grid_dim.a) * k * 0.5f)
                };
                DrawLine(x, 0, x, sh, c);
            } else {
                DrawLine(x, 0, x, sh, grid_dim);
            }
        }
        int gy0 = ((int)oy) % spacing;
        for (int y = gy0; y < sh; y += spacing) {
            DrawLine(0, y, sw, y, grid_dim);
        }
    }

    // --- Vignette outside play area ---
    if (ox > 0.5f) {
        DrawRectangle(0, 0, (int)ox, sh, (Color){0, 0, 0, 130});
        DrawRectangle((int)(ox + pw), 0, sw - (int)(ox + pw), sh,
                      (Color){0, 0, 0, 130});
    }
    if (oy > 0.5f) {
        DrawRectangle(0, 0, sw, (int)oy, (Color){0, 0, 0, 130});
        DrawRectangle(0, (int)(oy + ph), sw, sh - (int)(oy + ph),
                      (Color){0, 0, 0, 130});
    }

    // --- Play area border (neon) ---
    {
        float pad = 1.5f * scale;
        Rectangle r = {ox - pad, oy - pad, pw + pad * 2, ph + pad * 2};
        DrawRectangleLinesEx(r, 1.2f, (Color){60, 140, 200, 130});

        // Subtle inner glow line
        Rectangle r2 = {ox + 1, oy + 1, pw - 2, ph - 2};
        DrawRectangleLinesEx(r2, 1.0f, (Color){80, 170, 230, 35});
    }

    // --- Corner accent marks (L-shapes) ---
    {
        float pad = 1.5f * scale;
        float clen = 18.0f * scale;
        float thick = 2.5f * scale;
        float bx0 = ox - pad;
        float by0 = oy - pad;
        float bx1 = ox + pw + pad;
        float by1 = oy + ph + pad;
        Color c = {130, 220, 255, 230};

        DrawLineEx((Vector2){bx0, by0}, (Vector2){bx0 + clen, by0}, thick, c);
        DrawLineEx((Vector2){bx0, by0}, (Vector2){bx0, by0 + clen}, thick, c);
        DrawLineEx((Vector2){bx1, by0}, (Vector2){bx1 - clen, by0}, thick, c);
        DrawLineEx((Vector2){bx1, by0}, (Vector2){bx1, by0 + clen}, thick, c);
        DrawLineEx((Vector2){bx0, by1}, (Vector2){bx0 + clen, by1}, thick, c);
        DrawLineEx((Vector2){bx0, by1}, (Vector2){bx0, by1 - clen}, thick, c);
        DrawLineEx((Vector2){bx1, by1}, (Vector2){bx1 - clen, by1}, thick, c);
        DrawLineEx((Vector2){bx1, by1}, (Vector2){bx1, by1 - clen}, thick, c);
    }
}
