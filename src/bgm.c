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

static const int title_lead[] = {
    69, 0, 72, 0, 76, 0, 72, 0, 74, 0, 72, 0, 69, 0, 0, 0,
    71, 0, 74, 0, 77, 0, 74, 0, 76, 0, 74, 0, 71, 0, 0, 0
};
static const int title_bass[] = {
    33, 0, 0, 0, 33, 0, 0, 0, 33, 0, 0, 0, 33, 0, 0, 0,
    35, 0, 0, 0, 35, 0, 0, 0, 35, 0, 0, 0, 35, 0, 0, 0
};

static const int game_lead[] = {
    69, 72, 76, 72, 74, 71, 69, 67, 69, 72, 76, 72, 74, 76, 79, 76,
    69, 72, 76, 72, 74, 71, 69, 67, 69, 72, 76, 79, 81, 79, 76, 72
};
static const int game_bass[] = {
    45, 0, 45, 0, 45, 0, 45, 0, 43, 0, 43, 0, 43, 0, 43, 0,
    41, 0, 41, 0, 41, 0, 41, 0, 40, 0, 40, 0, 40, 0, 40, 0
};

static const int boss_lead[] = {
    72, 75, 79, 75, 72, 75, 79, 82, 80, 77, 73, 70, 73, 77, 80, 82,
    72, 75, 79, 82, 84, 82, 79, 75, 77, 80, 84, 80, 77, 75, 72, 70
};
static const int boss_bass[] = {
    36, 36, 36, 36, 36, 36, 36, 36, 39, 39, 39, 39, 39, 39, 39, 39,
    34, 34, 34, 34, 34, 34, 34, 34, 38, 38, 38, 38, 38, 38, 38, 38
};

typedef struct {
    const int *lead;
    const int *bass;
    int length;
    int bpm;
    float gain;
} Track;

static Track tracks[BGM_COUNT] = {
    {0, 0, 0, 0, 0},                                 // BGM_NONE
    {title_lead, title_bass, 32, 90, 0.10f},         // BGM_TITLE
    {game_lead, game_bass, 32, 130, 0.10f},          // BGM_GAME
    {boss_lead, boss_bass, 32, 160, 0.13f},          // BGM_BOSS
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
