#include "game.h"
#include <stdio.h>
#include <string.h>

#define SETTINGS_FILENAME "disk_survivor.cfg"
#define SETTINGS_MAGIC 0x44535347u  // 'DSSG'
#define SETTINGS_VERSION 1

typedef struct {
    unsigned int magic;
    unsigned int version;
    float sfx_volume;
    float bgm_volume;
    int fullscreen;
} SettingsFile;

static void make_path(char *out, int cap) {
    const char *dir = GetApplicationDirectory();
    if (dir && dir[0]) snprintf(out, cap, "%s%s", dir, SETTINGS_FILENAME);
    else snprintf(out, cap, "%s", SETTINGS_FILENAME);
}

void settings_load(Settings *s) {
    s->sfx_volume = 0.8f;
    s->bgm_volume = 0.6f;
    s->fullscreen = false;

    char path[1024];
    make_path(path, sizeof(path));
    if (!FileExists(path)) return;

    int size = 0;
    unsigned char *data = LoadFileData(path, &size);
    if (!data) return;
    if (size >= (int)sizeof(SettingsFile)) {
        SettingsFile f;
        memcpy(&f, data, sizeof(f));
        if (f.magic == SETTINGS_MAGIC && f.version == SETTINGS_VERSION) {
            s->sfx_volume = f.sfx_volume;
            s->bgm_volume = f.bgm_volume;
            s->fullscreen = (f.fullscreen != 0);
        }
    }
    UnloadFileData(data);
}

void settings_save(const Settings *s) {
    SettingsFile f = {0};
    f.magic = SETTINGS_MAGIC;
    f.version = SETTINGS_VERSION;
    f.sfx_volume = s->sfx_volume;
    f.bgm_volume = s->bgm_volume;
    f.fullscreen = s->fullscreen ? 1 : 0;

    char path[1024];
    make_path(path, sizeof(path));
    SaveFileData(path, &f, sizeof(f));
}

void settings_apply(const Settings *s) {
    audio_set_sfx_volume(s->sfx_volume);
    bgm_set_volume(s->bgm_volume);
    if (s->fullscreen != IsWindowFullscreen()) {
        ToggleFullscreen();
    }
}
