#include "game.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define SAMPLE_RATE 22050
#define MAX_SFX_LEN (SAMPLE_RATE / 2)

static Sound sfx[SFX_COUNT];
static bool audio_ready = false;

static float square(float t, float freq) {
    return sinf(t * freq * 2.0f * 3.14159f) >= 0 ? 1.0f : -1.0f;
}

static float saw(float t, float freq) {
    float p = t * freq;
    return 2.0f * (p - floorf(p)) - 1.0f;
}

static float noise(void) {
    return ((rand() % 2000) / 1000.0f) - 1.0f;
}

static Sound make_sound(short *samples, int count) {
    Wave w = {0};
    w.frameCount = count;
    w.sampleRate = SAMPLE_RATE;
    w.sampleSize = 16;
    w.channels = 1;
    w.data = samples;
    Sound s = LoadSoundFromWave(w);
    return s;
}

// Shoot: short square wave, falling pitch
static Sound gen_shoot(void) {
    int len = SAMPLE_RATE / 14;
    short *buf = malloc(len * sizeof(short));
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float p = (float)i / len;
        float freq = 880.0f - p * 400.0f;
        float env = (1.0f - p);
        float s = square(t, freq) * env * 0.18f;
        buf[i] = (short)(s * 32767);
    }
    Sound snd = make_sound(buf, len);
    free(buf);
    return snd;
}

// Enemy hit: short noise burst
static Sound gen_enemy_hit(void) {
    int len = SAMPLE_RATE / 20;
    short *buf = malloc(len * sizeof(short));
    for (int i = 0; i < len; i++) {
        float p = (float)i / len;
        float env = (1.0f - p) * (1.0f - p);
        float s = noise() * env * 0.25f;
        buf[i] = (short)(s * 32767);
    }
    Sound snd = make_sound(buf, len);
    free(buf);
    return snd;
}

// Enemy die: descending tone + noise
static Sound gen_enemy_die(void) {
    int len = SAMPLE_RATE / 8;
    short *buf = malloc(len * sizeof(short));
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float p = (float)i / len;
        float freq = 440.0f - p * 300.0f;
        float env = (1.0f - p);
        float s = (square(t, freq) * 0.5f + noise() * 0.3f) * env * 0.22f;
        buf[i] = (short)(s * 32767);
    }
    Sound snd = make_sound(buf, len);
    free(buf);
    return snd;
}

// Player hit: low rumble + noise
static Sound gen_player_hit(void) {
    int len = SAMPLE_RATE / 4;
    short *buf = malloc(len * sizeof(short));
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float p = (float)i / len;
        float env = (1.0f - p);
        float s = (saw(t, 80.0f - p * 30.0f) * 0.6f + noise() * 0.4f) * env * 0.35f;
        buf[i] = (short)(s * 32767);
    }
    Sound snd = make_sound(buf, len);
    free(buf);
    return snd;
}

// Level up: ascending arpeggio (3 notes)
static Sound gen_level_up(void) {
    int len = SAMPLE_RATE / 3;
    short *buf = malloc(len * sizeof(short));
    float notes[3] = {523.25f, 659.25f, 783.99f};
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float p = (float)i / len;
        int note_idx = (int)(p * 3.0f);
        if (note_idx > 2) note_idx = 2;
        float note_p = (p * 3.0f) - note_idx;
        float env = (1.0f - note_p) * 0.8f + 0.2f;
        if (p > 0.85f) env *= (1.0f - (p - 0.85f) / 0.15f);
        float s = square(t, notes[note_idx]) * env * 0.18f;
        buf[i] = (short)(s * 32767);
    }
    Sound snd = make_sound(buf, len);
    free(buf);
    return snd;
}

// Gem pickup: very short high ping
static Sound gen_gem_pickup(void) {
    int len = SAMPLE_RATE / 25;
    short *buf = malloc(len * sizeof(short));
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float p = (float)i / len;
        float env = (1.0f - p);
        float s = square(t, 1320.0f) * env * 0.1f;
        buf[i] = (short)(s * 32767);
    }
    Sound snd = make_sound(buf, len);
    free(buf);
    return snd;
}

// Boss spawn: low rumble going up
static Sound gen_boss_spawn(void) {
    int len = SAMPLE_RATE * 4 / 5;
    short *buf = malloc(len * sizeof(short));
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float p = (float)i / len;
        float freq = 60.0f + p * 60.0f;
        float env = sinf(p * 3.14159f);
        float s = (saw(t, freq) * 0.7f + noise() * 0.3f) * env * 0.35f;
        buf[i] = (short)(s * 32767);
    }
    Sound snd = make_sound(buf, len);
    free(buf);
    return snd;
}

// Boss hit: heavy hit
static Sound gen_boss_hit(void) {
    int len = SAMPLE_RATE / 12;
    short *buf = malloc(len * sizeof(short));
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float p = (float)i / len;
        float env = (1.0f - p) * (1.0f - p);
        float s = (square(t, 160.0f) * 0.5f + noise() * 0.5f) * env * 0.3f;
        buf[i] = (short)(s * 32767);
    }
    Sound snd = make_sound(buf, len);
    free(buf);
    return snd;
}

// Nova: descending sweep
static Sound gen_nova(void) {
    int len = SAMPLE_RATE / 3;
    short *buf = malloc(len * sizeof(short));
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float p = (float)i / len;
        float freq = 600.0f - p * 400.0f;
        float env = (1.0f - p);
        float s = (saw(t, freq) * 0.5f + square(t, freq * 1.5f) * 0.5f) * env * 0.22f;
        buf[i] = (short)(s * 32767);
    }
    Sound snd = make_sound(buf, len);
    free(buf);
    return snd;
}

// Beam: zap
static Sound gen_beam(void) {
    int len = SAMPLE_RATE / 3;
    short *buf = malloc(len * sizeof(short));
    for (int i = 0; i < len; i++) {
        float t = (float)i / SAMPLE_RATE;
        float p = (float)i / len;
        float freq = 200.0f + sinf(p * 30.0f) * 100.0f;
        float env = (1.0f - p);
        float s = saw(t, freq) * env * 0.2f;
        buf[i] = (short)(s * 32767);
    }
    Sound snd = make_sound(buf, len);
    free(buf);
    return snd;
}

void audio_init(void) {
    InitAudioDevice();
    if (!IsAudioDeviceReady()) return;

    sfx[SFX_SHOOT] = gen_shoot();
    sfx[SFX_ENEMY_HIT] = gen_enemy_hit();
    sfx[SFX_ENEMY_DIE] = gen_enemy_die();
    sfx[SFX_PLAYER_HIT] = gen_player_hit();
    sfx[SFX_LEVEL_UP] = gen_level_up();
    sfx[SFX_GEM_PICKUP] = gen_gem_pickup();
    sfx[SFX_BOSS_SPAWN] = gen_boss_spawn();
    sfx[SFX_BOSS_HIT] = gen_boss_hit();
    sfx[SFX_NOVA] = gen_nova();
    sfx[SFX_BEAM] = gen_beam();

    audio_ready = true;
}

void audio_play(SfxId id) {
    if (!audio_ready) return;
    if (id < 0 || id >= SFX_COUNT) return;
    PlaySound(sfx[id]);
}

void audio_cleanup(void) {
    if (!audio_ready) return;
    for (int i = 0; i < SFX_COUNT; i++) UnloadSound(sfx[i]);
    CloseAudioDevice();
    audio_ready = false;
}

void audio_set_sfx_volume(float v) {
    if (!audio_ready) return;
    if (v < 0) v = 0;
    if (v > 1) v = 1;
    for (int i = 0; i < SFX_COUNT; i++) SetSoundVolume(sfx[i], v);
}
