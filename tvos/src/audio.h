/* KEEL OS — procedural music engine, spectrum analysis, UI sounds. */
#pragma once

enum { SFX_MOVE, SFX_SELECT, SFX_BACK };

void audio_init(int enable_output);
void audio_shutdown(void);
void audio_play(int album, int track);
void audio_stop(void);
void audio_toggle_pause(void);
void audio_next(void);
void audio_prev(void);
void audio_seek(float seconds);
int audio_playing(void);   /* a track is loaded (maybe paused) */
int audio_paused(void);
int audio_album(void);
int audio_track(void);
float audio_position(void);
float audio_duration(void);
float audio_track_length(int album, int track);
/* n log-spaced bins in [0,1], smoothed. */
void audio_spectrum(float *bins, int n);
void audio_waveform(float *out, int n);
float audio_level(void);
float audio_beat(void);    /* decays from 1 after each kick */
const char *audio_device(void);
void sfx_play(int id);
/* Advance the audio clock manually (headless deterministic mode). */
void audio_set_manual_clock(int on);
void audio_tick(float dt);
