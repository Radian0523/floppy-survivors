#include "game.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SCORE_FILENAME "disk_survivor.sav"
#define SCORE_MAGIC 0x44534B53u  // 'DSKS' little-endian
#define SCORE_VERSION 1

typedef struct {
    unsigned int magic;
    unsigned int version;
    float best_time;
    int best_kills;
    int best_level;
    int boss_defeats;
    int total_games;
} ScoreFile;

static void make_path(char *out, int cap) {
    const char *dir = GetApplicationDirectory();
    if (dir && dir[0]) {
        snprintf(out, cap, "%s%s", dir, SCORE_FILENAME);
    } else {
        snprintf(out, cap, "%s", SCORE_FILENAME);
    }
}

void score_load(BestScore *s) {
    s->best_time = 0;
    s->best_kills = 0;
    s->best_level = 0;
    s->boss_defeats = 0;
    s->total_games = 0;

    char path[1024];
    make_path(path, sizeof(path));
    if (!FileExists(path)) return;

    int size = 0;
    unsigned char *data = LoadFileData(path, &size);
    if (!data) return;

    if (size >= (int)sizeof(ScoreFile)) {
        ScoreFile f;
        memcpy(&f, data, sizeof(ScoreFile));
        if (f.magic == SCORE_MAGIC && f.version == SCORE_VERSION) {
            s->best_time = f.best_time;
            s->best_kills = f.best_kills;
            s->best_level = f.best_level;
            s->boss_defeats = f.boss_defeats;
            s->total_games = f.total_games;
        }
    }
    UnloadFileData(data);
}

void score_save(const BestScore *s) {
    ScoreFile f = {0};
    f.magic = SCORE_MAGIC;
    f.version = SCORE_VERSION;
    f.best_time = s->best_time;
    f.best_kills = s->best_kills;
    f.best_level = s->best_level;
    f.boss_defeats = s->boss_defeats;
    f.total_games = s->total_games;

    char path[1024];
    make_path(path, sizeof(path));
    SaveFileData(path, &f, sizeof(f));
}

int score_update(BestScore *s, float time, int kills, int level, bool boss_killed) {
    int flags = 0;
    s->total_games++;
    if (time > s->best_time) { s->best_time = time; flags |= 1; }
    if (kills > s->best_kills) { s->best_kills = kills; flags |= 2; }
    if (level > s->best_level) { s->best_level = level; flags |= 4; }
    if (boss_killed) {
        if (s->boss_defeats == 0) flags |= 8;
        s->boss_defeats++;
    }
    score_save(s);
    return flags;
}
