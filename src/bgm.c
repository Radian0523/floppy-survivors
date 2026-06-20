#include "game.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define BGM_SAMPLE_RATE 22050
#define BGM_STREAM_BUFFER 1024

static AudioStream stream;
static bool stream_ready = false;
static BgmId current = BGM_NONE;
static unsigned long sample_pos = 0;

// Music data: each entry is a MIDI note number (0 = rest)
// Pattern is in 16th notes; tempo controls playback rate.
// Notes: A4=69, C5=72, etc.

// Drum hit codes: 0=none, 1=kick, 2=snare, 3=hat (short noise burst)
static const int title_lead[] = {
    69, 0, 72, 0, 76, 0, 72, 0, 74, 0, 72, 0, 69, 0, 0, 0,
    71, 0, 74, 0, 77, 0, 74, 0, 76, 0, 74, 0, 71, 0, 0, 0
};
static const int title_bass[] = {
    33, 0, 0, 0, 33, 0, 0, 0, 33, 0, 0, 0, 33, 0, 0, 0,
    35, 0, 0, 0, 35, 0, 0, 0, 35, 0, 0, 0, 35, 0, 0, 0
};
static const int title_drum[] = {
    1, 0, 3, 0, 1, 0, 3, 0, 1, 0, 3, 0, 1, 0, 3, 0,
    1, 0, 3, 0, 1, 0, 3, 0, 1, 0, 3, 0, 1, 0, 3, 0
};

static const int game_lead[] = {
    69, 72, 76, 72, 74, 71, 69, 67, 69, 72, 76, 72, 74, 76, 79, 76,
    69, 72, 76, 72, 74, 71, 69, 67, 69, 72, 76, 79, 81, 79, 76, 72
};
static const int game_bass[] = {
    45, 0, 45, 0, 45, 0, 45, 0, 43, 0, 43, 0, 43, 0, 43, 0,
    41, 0, 41, 0, 41, 0, 41, 0, 40, 0, 40, 0, 40, 0, 40, 0
};
static const int game_drum[] = {
    1, 0, 3, 0, 2, 0, 3, 0, 1, 0, 3, 0, 2, 0, 3, 3,
    1, 0, 3, 0, 2, 0, 3, 0, 1, 0, 3, 0, 2, 0, 3, 3
};

// Late-game: faster, denser, more urgent
static const int game_late_lead[] = {
    69, 72, 76, 79, 76, 72, 69, 72, 71, 74, 77, 81, 77, 74, 71, 74,
    72, 76, 79, 84, 79, 76, 72, 76, 74, 77, 81, 86, 81, 77, 74, 77
};
static const int game_late_bass[] = {
    45, 45, 0, 45, 43, 43, 0, 43, 41, 41, 0, 41, 40, 40, 0, 40,
    45, 45, 0, 45, 43, 43, 0, 43, 41, 41, 0, 41, 40, 40, 38, 38
};
static const int game_late_drum[] = {
    1, 3, 1, 3, 2, 3, 1, 3, 1, 3, 1, 3, 2, 3, 1, 3,
    1, 3, 1, 3, 2, 3, 1, 3, 1, 3, 1, 3, 2, 3, 2, 1
};

static const int boss_lead[] = {
    72, 75, 79, 75, 72, 75, 79, 82, 80, 77, 73, 70, 73, 77, 80, 82,
    72, 75, 79, 82, 84, 82, 79, 75, 77, 80, 84, 80, 77, 75, 72, 70
};
static const int boss_bass[] = {
    36, 36, 36, 36, 36, 36, 36, 36, 39, 39, 39, 39, 39, 39, 39, 39,
    34, 34, 34, 34, 34, 34, 34, 34, 38, 38, 38, 38, 38, 38, 38, 38
};
static const int boss_drum[] = {
    1, 3, 1, 3, 2, 3, 1, 3, 1, 3, 1, 3, 2, 3, 2, 1,
    1, 3, 1, 3, 2, 3, 1, 3, 1, 3, 1, 3, 2, 3, 2, 1
};

typedef struct {
    const int *lead;
    const int *bass;
    const int *drum;
    int length;
    int bpm;
    float gain;
} Track;

static Track tracks[BGM_COUNT] = {
    {0, 0, 0, 0, 0, 0},                                              // BGM_NONE
    {title_lead, title_bass, title_drum, 32, 90, 0.10f},             // BGM_TITLE
    {game_lead, game_bass, game_drum, 32, 130, 0.10f},               // BGM_GAME
    {game_late_lead, game_late_bass, game_late_drum, 32, 145, 0.11f},// BGM_GAME_LATE
    {boss_lead, boss_bass, boss_drum, 32, 160, 0.13f},               // BGM_BOSS
};

static float midi_to_freq(int note) {
    if (note <= 0) return 0;
    return 440.0f * powf(2.0f, (note - 69) / 12.0f);
}

static float square_wave(float phase) {
    return (phase < 0.5f) ? 1.0f : -1.0f;
}

static float triangle_wave(float phase) {
    if (phase < 0.5f) return -1.0f + 4.0f * phase;
    return 3.0f - 4.0f * phase;
}

// Simple LCG noise (deterministic per-sample, no state)
static float noise_at(unsigned long n) {
    n = n * 1103515245UL + 12345UL;
    return ((float)((n >> 16) & 0x7FFF) / 16383.5f) - 1.0f;
}

// Kick: low sine sweep + decay (typical analog kick)
static float drum_kick(float t) {
    if (t > 0.18f) return 0;
    float env = 1.0f - t / 0.18f;
    float freq = 120.0f - 80.0f * (t / 0.18f);  // sweep down
    float phase = fmodf(t * freq, 1.0f);
    float s = sinf(phase * 6.2831853f);
    return s * env * env;
}

// Snare: noise burst with tonal body
static float drum_snare(float t, unsigned long sample_n) {
    if (t > 0.14f) return 0;
    float env = 1.0f - t / 0.14f;
    float n = noise_at(sample_n);
    float tone = sinf(t * 380.0f * 6.2831853f) * 0.3f;
    return (n * 0.8f + tone) * env * env;
}

// Hi-hat: short noise with high-pass-ish feel
static float drum_hat(float t, unsigned long sample_n) {
    if (t > 0.06f) return 0;
    float env = 1.0f - t / 0.06f;
    float n = noise_at(sample_n) - noise_at(sample_n - 1) * 0.5f;
    return n * env * env;
}

static void audio_callback(void *buffer, unsigned int frames) {
    short *out = (short *)buffer;
    Track *t = &tracks[current];

    if (current == BGM_NONE || t->length == 0) {
        memset(out, 0, frames * sizeof(short));
        return;
    }

    float beat_dur = 60.0f / t->bpm;
    float step_dur = beat_dur / 4.0f;  // 16th notes
    float pattern_dur = step_dur * t->length;

    for (unsigned int i = 0; i < frames; i++) {
        float time = (float)(sample_pos + i) / BGM_SAMPLE_RATE;
        float pat_time = fmodf(time, pattern_dur);
        int step = (int)(pat_time / step_dur);
        if (step >= t->length) step = t->length - 1;
        float step_pos = pat_time - step * step_dur;
        float step_progress = step_pos / step_dur;

        // Simple envelope: fast attack, gentle decay
        float env = 1.0f - step_progress * 0.7f;
        if (step_progress < 0.05f) env = step_progress / 0.05f;

        float sample = 0;

        // Lead voice (square)
        int lead_note = t->lead[step];
        if (lead_note > 0) {
            float freq = midi_to_freq(lead_note);
            float phase = fmodf(time * freq, 1.0f);
            sample += square_wave(phase) * 0.45f * env;
        }

        // Bass voice (triangle, less envelope)
        int bass_note = t->bass[step];
        if (bass_note > 0) {
            float freq = midi_to_freq(bass_note);
            float phase = fmodf(time * freq, 1.0f);
            float bass_env = 1.0f - step_progress * 0.3f;
            sample += triangle_wave(phase) * 0.55f * bass_env;
        }

        // Drum voice
        if (t->drum) {
            int drum = t->drum[step];
            float drum_sample = 0;
            unsigned long sn = sample_pos + i;
            switch (drum) {
                case 1: drum_sample = drum_kick(step_pos) * 0.95f; break;
                case 2: drum_sample = drum_snare(step_pos, sn) * 0.55f; break;
                case 3: drum_sample = drum_hat(step_pos, sn) * 0.30f; break;
                default: break;
            }
            sample += drum_sample;
        }

        sample *= t->gain;
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;

        out[i] = (short)(sample * 32767);
    }

    sample_pos += frames;
}

void bgm_init(void) {
    if (!IsAudioDeviceReady()) return;
    stream = LoadAudioStream(BGM_SAMPLE_RATE, 16, 1);
    SetAudioStreamCallback(stream, audio_callback);
    SetAudioStreamVolume(stream, 0.6f);
    PlayAudioStream(stream);
    stream_ready = true;
    current = BGM_NONE;
}

void bgm_play(BgmId id) {
    if (!stream_ready) return;
    if (id == current) return;
    if (id < 0 || id >= BGM_COUNT) return;
    current = id;
    sample_pos = 0;
}

void bgm_stop(void) {
    if (!stream_ready) return;
    current = BGM_NONE;
}

void bgm_cleanup(void) {
    if (!stream_ready) return;
    StopAudioStream(stream);
    UnloadAudioStream(stream);
    stream_ready = false;
}

void bgm_set_volume(float v) {
    if (!stream_ready) return;
    if (v < 0) v = 0;
    if (v > 1) v = 1;
    SetAudioStreamVolume(stream, v);
}
