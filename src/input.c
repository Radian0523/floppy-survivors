#include "game.h"
#include <math.h>

Vector2 input_get_move_direction(void) {
    Vector2 dir = {0, 0};

    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) dir.y -= 1;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) dir.y += 1;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) dir.x -= 1;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) dir.x += 1;

    float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
    if (len > 0) {
        dir.x /= len;
        dir.y /= len;
    }
    return dir;
}

bool input_is_mouse_active(void) {
    return IsMouseButtonDown(MOUSE_BUTTON_LEFT);
}

Vector2 input_get_mouse_direction(Vector2 player_screen_pos) {
    Vector2 mouse = GetMousePosition();
    Vector2 dir = {mouse.x - player_screen_pos.x, mouse.y - player_screen_pos.y};
    float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
    if (len > 10.0f) {
        dir.x /= len;
        dir.y /= len;
    } else {
        dir.x = 0;
        dir.y = 0;
    }
    return dir;
}
