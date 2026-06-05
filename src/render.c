#include "game.h"

void render_background(void) {
    ClearBackground((Color){5, 5, 15, 255});

    int spacing = 40;
    Color grid_color = {20, 30, 50, 100};
    for (int x = 0; x < GetScreenWidth(); x += spacing) {
        DrawLine(x, 0, x, GetScreenHeight(), grid_color);
    }
    for (int y = 0; y < GetScreenHeight(); y += spacing) {
        DrawLine(0, y, GetScreenWidth(), y, grid_color);
    }
}
