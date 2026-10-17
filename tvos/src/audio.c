/* KEEL OS — procedural music engine.
 * Every album is a generative "style": a step sequencer drives drums, bass,
 * pads and lead voices built from basic oscillators, envelopes, filters, a
 * stereo delay and a small Schroeder reverb. Audio is streamed to a system
 * player (pw-cat / pacat / aplay) when available; the analyser (FFT) runs on
 * the same samples so the visualizer always matches what's heard. */
#define _GNU_SOURCE
#include "audio.h"
#include "assets.h"
#include "core.h"
#include <pthread.h>
#include <unistd.h>

#define SR 44100
#define RING 65536

typedef struct {
    int on, type;          /* 0 saw 1 square 2 tri 3 sine 4 epiano 5 pluck */
    float freq, phase, phase2, detune;
    float env, a, d, s, r;  /* attack rate, decay coef, sustain, release coef */
    int stage, len;         /* samples until release */
    float cut, cut_env, lp, lp2, res;
    float gain, pan, send;
} Voice;

typedef struct {
    int style, bpm, root, minor;
    uint32_t seed;
    int prog[4];
    int motif[32];
    int kick, snare, hat, ohat; /* 16-bit patterns */
    int bass_pat;
    float swing;
    /* state */
    int64_t sample;
    int step;
    double next_step;
    Voice v[24];
    float kick_env, kick_ph, kick_f;
    float sn_env, hat_env, ohat_env, clap_env;
    float hp_prev, hp_out;
    float dl_l[SR], dl_r[SR];
    int dl_pos;
    float comb[4][2048], comb_fb[4];
    int comb_pos[4];
    float ap[2][1024];
    int ap_pos[2];
    float master_lp_l, master_lp_r;
    uint32_t noise;
    float beat;
} Synth;

float audio_track_length(int album, int track) { return 140.f + (float)((album * 37 + track * 53) % 90); }
#define TRACK_SECONDS (A.dur)

static const int SCALE_MAJ[7] = {0, 2, 4, 5, 7, 9, 11}, SCALE_MIN[7] = {0, 2, 3, 5, 7, 8, 10};
static const int COMB_LEN[4] = {1116, 1188, 1277, 1356}, AP_LEN[2] = {556, 441};

static struct {
    pthread_t th;
    pthread_mutex_t m;
    int running, album, track, playing, paused, manual;
    double pos;           /* heard position in seconds */
    int64_t gen;          /* samples generated for this track */
    float ring[RING][2];
    Synth syn;
    FILE *out;
    char dev[64];
    float spec[128];
    float beat_vis;
    int sfx_req;
    float sfx_env, sfx_ph, sfx_f;
    int sfx_kind;
    double last;
    float dur;
} A;

static inline float noisef(Synth *s) { s->noise = s->noise * 1664525u + 1013904223u; return ((s->noise >> 9) & 0x7FFF) / 16384.f - 1.f; }
static inline float mtof(float m) { return 440.f * powf(2.f, (m - 69.f) / 12.f); }

static int scale_note(Synth *s, int degree) {
    const int *sc = s->minor ? SCALE_MIN : SCALE_MAJ;
    int oct = degree >= 0 ? degree / 7 : (degree - 6) / 7;
    int d = degree - oct * 7;
    return s->root + sc[d] + 12 * oct;
}

static void synth_setup(Synth *s, int album, int track) {
    const Album *al = &g_albums[album];
    memset(s, 0, sizeof *s);
    s->style = al->style;
    s->seed = (uint32_t)(album * 7919 + track * 104729 + 17);
    Rng rg = rng_make(s->seed);
    s->bpm = al->bpm + (track % 3) * 2 - 2;
    s->root = al->root + ((int[]){0, 2, -3, 5, -2})[track % 5];
    s->minor = al->minor ? (track != 3) : (track == 2);
    static const int PM[4][4] = {{0, 5, 2, 6}, {0, 3, 4, 0}, {0, 5, 3, 4}, {5, 3, 0, 4}};
    static const int PJ[4][4] = {{0, 4, 5, 3}, {0, 5, 3, 4}, {3, 4, 0, 5}, {0, 3, 0, 4}};
    int pi = rng_int(&rg, 4);
    memcpy(s->prog, s->minor ? PM[pi] : PJ[pi], sizeof s->prog);
    for (int i = 0; i < 32; i++) {
        int r = rng_int(&rg, 10);
        s->motif[i] = (r < 4) ? -99 : (rng_int(&rg, 8) + (i % 8 == 0 ? 0 : 0));
        if (i >= 16 && rng_int(&rg, 3)) s->motif[i] = s->motif[i - 16];
    }
    switch (s->style) {
    case 0: s->kick = 0x1111; s->snare = 0x0808 >> 0; s->hat = 0x5555; s->ohat = 0x0000; s->bass_pat = 0xFFFF; break;
    case 1: s->kick = 0x0125; s->snare = 0x0808; s->hat = 0x5555; s->swing = 0.18f; s->bass_pat = 0x0121; break;
    case 2: s->kick = 0; s->snare = 0; s->hat = 0; s->bass_pat = 0x0101; break;
    case 3: s->kick = 0x1111; s->snare = 0x0808; s->hat = 0xFFFF & 0x7777; s->ohat = 0x4444; s->bass_pat = 0x4444 | 0x0202; break;
    case 4: s->kick = 0; s->snare = 0; s->hat = 0; s->bass_pat = 0x0101; break;
    case 5: s->kick = 0x1111; s->snare = 0x0808; s->hat = 0xAAAA; s->bass_pat = 0x5555; break;
    case 6: s->kick = 0x0101; s->snare = 0; s->hat = 0; s->bass_pat = 0x0101; break;
    default: s->kick = 0x0129; s->snare = 0x0808; s->hat = 0xFFFF; s->ohat = 0x2020; s->bass_pat = 0x2D29; s->swing = 0.08f; break;
    }
    for (int i = 0; i < 4; i++) s->comb_fb[i] = s->style == 2 || s->style == 6 ? 0.86f : 0.78f;
    s->noise = s->seed | 1;
    s->next_step = 0;
}

static Voice *voice(Synth *s) {
    Voice *best = &s->v[0];
    for (int i = 0; i < 24; i++) {
        if (!s->v[i].on) return &s->v[i];
        if (s->v[i].env < best->env) best = &s->v[i];
    }
    return best;
}
static void note(Synth *s, int type, int midi, float len_steps, float gain, float a_ms, float d_s, float sus, float r_s,
                 float cut, float cut_env, float pan, float send, float detune) {
    Voice *v = voice(s);
    double sps = SR * 60.0 / s->bpm / 4.0;
    memset(v, 0, sizeof *v);
    v->on = 1; v->type = type; v->freq = mtof((float)midi);
    v->a = 1.f / fmaxf(1.f, a_ms * SR / 1000.f);
    v->d = expf(-1.f / fmaxf(1.f, d_s * SR));
    v->s = sus;
    v->r = expf(-1.f / fmaxf(1.f, r_s * SR));
    v->len = (int)(len_steps * sps);
    v->cut = cut; v->cut_env = cut_env;
    v->gain = gain; v->pan = pan; v->send = send; v->detune = detune;
    v->phase = (s->noise & 1023) / 1024.f;
    v->phase2 = 0.37f;
}

static void step_event(Synth *s, int step) {
    int bar = step / 16, st = step % 16;
    int chord = s->prog[bar % 4];
    int root = scale_note(s, chord);
    int third = scale_note(s, chord + 2), fifth = scale_note(s, chord + 4), seventh = scale_note(s, chord + 6);
    int bit = 1 << (15 - st);
    int intro = bar < 2;
    if ((s->kick & bit) && !intro) { s->kick_env = 1.f; s->kick_ph = 0; s->kick_f = s->style == 6 ? 90.f : 150.f; s->beat = 1.f; }
    if ((s->snare & bit) && !intro) s->sn_env = 1.f;
    if ((s->hat & bit) && bar >= 1) s->hat_env = 1.f;
    if ((s->ohat & bit) && !intro) s->ohat_env = 1.f;
    switch (s->style) {
    case 0: /* synthwave */
        if (s->bass_pat & bit) note(s, 0, root - 24 + ((st & 2) ? 12 : 0), 0.9f, 0.32f, 2, 0.15f, 0.4f, 0.05f, 400, 2200, 0, 0.05f, 0);
        if (st == 0) { int ch[4] = {root - 12, third - 12, fifth - 12, root}; for (int i = 0; i < 4; i++) note(s, 0, ch[i], 16, 0.07f, 300, 2.f, 0.8f, 0.6f, 1600, 600, (i - 1.5f) * 0.3f, 0.35f, 0.006f); }
        if (bar >= 2) { int arp[4] = {root, third, fifth, root + 12}; note(s, 1, arp[st % 4] + 12, 0.8f, 0.05f, 1, 0.12f, 0.2f, 0.08f, 3000, 2000, (st % 2) ? 0.4f : -0.4f, 0.3f, 0); }
        break;
    case 1: /* lofi */
        if (s->bass_pat & bit) note(s, 3, root - 24, 3, 0.35f, 5, 0.6f, 0.5f, 0.1f, 900, 0, 0, 0.05f, 0);
        if (st == 0 || st == 10) { int ch[4] = {root - 12, third - 12, fifth - 12, seventh - 12}; for (int i = 0; i < 4; i++) note(s, 4, ch[i], 6, 0.09f, 8, 1.2f, 0.3f, 0.4f, 2200, 0, (i - 1.5f) * 0.25f, 0.3f, 0); }
        if (bar % 4 >= 2 && s->motif[step % 32] >= 0 && (st % 2 == 0)) note(s, 4, scale_note(s, s->motif[step % 32] + 7), 2, 0.06f, 5, 0.8f, 0.2f, 0.3f, 2600, 0, 0.3f, 0.4f, 0);
        break;
    case 2: /* ambient */
        if (st == 0 && bar % 2 == 0) { int ch[5] = {root - 24, root - 12, fifth - 12, third, fifth}; for (int i = 0; i < 5; i++) note(s, 0, ch[i], 32, 0.05f, 1500, 4.f, 0.8f, 3.f, 900, 700, (i - 2) * 0.35f, 0.7f, 0.008f); }
        if (s->motif[step % 32] >= 0 && st % 4 == 0 && bar >= 1) note(s, 3, scale_note(s, s->motif[step % 32] + 14), 4, 0.05f, 4, 1.8f, 0.0f, 1.5f, 8000, 0, (s->motif[step % 32] % 3 - 1) * 0.5f, 0.8f, 0);
        break;
    case 3: /* house */
        if (s->bass_pat & bit) note(s, 0, root - 24, 1.2f, 0.3f, 2, 0.12f, 0.3f, 0.04f, 350, 1600, 0, 0.0f, 0);
        if ((st == 2 || st == 6 || st == 10 || st == 14) && bar >= 2) { int ch[3] = {root, third, fifth}; for (int i = 0; i < 3; i++) note(s, 0, ch[i], 0.7f, 0.06f, 2, 0.12f, 0.1f, 0.06f, 1200, 2400, (i - 1) * 0.35f, 0.3f, 0.004f); }
        break;
    case 4: /* piano */
        if (st % 2 == 0) { int ar[8] = {root - 12, fifth - 12, root, third, fifth, third, root, fifth - 12}; note(s, 4, ar[(st / 2) % 8], 6, 0.11f, 3, 1.6f, 0.0f, 0.8f, 5000, 0, ((st / 2) % 2 ? 0.3f : -0.3f), 0.45f, 0); }
        if (st == 0) note(s, 4, root - 24, 16, 0.12f, 3, 3.f, 0.0f, 1.f, 3000, 0, 0, 0.4f, 0);
        if (bar >= 2 && s->motif[step % 32] >= 0 && st % 4 == 2) note(s, 4, scale_note(s, s->motif[step % 32] + 14), 4, 0.1f, 2, 1.4f, 0, 0.8f, 7000, 0, 0.15f, 0.5f, 0);
        break;
    case 5: /* chiptune */
        if (s->bass_pat & bit) note(s, 2, root - 12 + ((st & 4) ? 7 : 0), 0.9f, 0.22f, 1, 0.2f, 0.6f, 0.02f, 9000, 0, 0, 0, 0);
        { int arp[3] = {root, third, fifth}; note(s, 1, arp[st % 3] + 12, 0.5f, 0.045f, 1, 0.08f, 0.5f, 0.02f, 9000, 0, -0.3f, 0.1f, 0); }
        if (bar >= 2 && s->motif[step % 32] >= 0 && st % 2 == 0) note(s, 1, scale_note(s, s->motif[step % 32] + 14), 1.6f, 0.05f, 1, 0.2f, 0.6f, 0.04f, 9000, 0, 0.3f, 0.15f, 0);
        break;
    case 6: /* cinematic */
        if (st == 0) { int ch[6] = {root - 24, root - 12, fifth - 12, root, third, fifth}; for (int i = 0; i < 6; i++) note(s, 0, ch[i], 16, 0.045f, 900, 3.f, 0.85f, 1.5f, 1400, 500, (i - 2.5f) * 0.3f, 0.6f, 0.007f); }
        if (bar >= 2 && st % 8 == 0 && s->motif[step % 32] >= 0) note(s, 3, scale_note(s, s->motif[step % 32] + 14), 8, 0.06f, 3, 2.5f, 0, 1.5f, 9000, 0, 0.2f, 0.7f, 0);
        break;
    default: /* funk */
        if (s->bass_pat & bit) note(s, 1, root - 24 + ((st % 8 == 6) ? 12 : 0), 0.6f, 0.26f, 1, 0.1f, 0.2f, 0.03f, 500, 2600, 0, 0, 0);
        if ((st % 4 == 2 || st == 7 || st == 15) && bar >= 1) { int ch[3] = {third, fifth, seventh}; for (int i = 0; i < 3; i++) note(s, 1, ch[i], 0.3f, 0.035f, 1, 0.05f, 0.0f, 0.02f, 2400, 1800, (i - 1) * 0.4f, 0.15f, 0); }
        if (bar % 4 == 3 && s->motif[step % 32] >= 0 && st % 2 == 0) note(s, 0, scale_note(s, s->motif[step % 32] + 14), 1, 0.05f, 2, 0.2f, 0.4f, 0.1f, 3000, 1500, 0.3f, 0.3f, 0.005f);
        break;
    }
}

static inline float osc(Voice *v, float ph) {
    switch (v->type) {
    case 0: return 2.f * ph - 1.f;
    case 1: return ph < 0.5f ? 0.8f : -0.8f;
    case 2: return 4.f * fabsf(ph - 0.5f) - 1.f;
    case 4: return sinf(2 * PI_F * ph + 0.6f * v->env * sinf(2 * PI_F * ph * 2.f));
    default: return sinf(2 * PI_F * ph);
    }
}

static void synth_render(Synth *s, float *out, int n) {
    double sps = SR * 60.0 / s->bpm / 4.0;
    for (int i = 0; i < n; i++) {
        while (s->sample >= (int64_t)s->next_step) {
            step_event(s, s->step);
            s->step++;
            double sw = (s->step % 2) ? s->swing : -s->swing;
            s->next_step = s->step * sps + sw * sps * 0.5;
        }
        float l = 0, r = 0, send = 0;
        for (int k = 0; k < 24; k++) {
            Voice *v = &s->v[k];
            if (!v->on) continue;
            if (v->stage == 0) { v->env += v->a; if (v->env >= 1.f) { v->env = 1.f; v->stage = 1; } }
            else if (v->stage == 1) { v->env = v->s + (v->env - v->s) * v->d; }
            else { v->env *= v->r; if (v->env < 0.0005f) { v->on = 0; continue; } }
            if (v->stage < 2 && --v->len <= 0) v->stage = 2;
            float inc = v->freq / SR;
            v->phase += inc; if (v->phase >= 1.f) v->phase -= 1.f;
            float x = osc(v, v->phase);
            if (v->detune > 0) {
                v->phase2 += inc * (1.f + v->detune); if (v->phase2 >= 1.f) v->phase2 -= 1.f;
                x = 0.6f * (x + osc(v, v->phase2));
            }
            float cut = v->cut + v->cut_env * v->env;
            float k1 = 1.f - expf(-2.f * PI_F * fminf(cut, 16000.f) / SR);
            v->lp += k1 * (x - v->lp);
            v->lp2 += k1 * (v->lp - v->lp2);
            float y = v->lp2 * v->env * v->gain;
            l += y * (1.f - fmaxf(0, v->pan) * 0.8f);
            r += y * (1.f + fminf(0, v->pan) * 0.8f);
            send += y * v->send;
        }
        /* drums */
        if (s->kick_env > 0.0005f) {
            s->kick_f = 45.f + (s->kick_f - 45.f) * 0.9992f;
            s->kick_ph += s->kick_f / SR;
            float k = sinf(2 * PI_F * s->kick_ph) * s->kick_env * (s->style == 5 ? 0.45f : 0.75f);
            s->kick_env *= s->style == 6 ? 0.99985f : 0.9997f;
            l += k; r += k;
        }
        float nz = noisef(s);
        float hp = nz - s->hp_prev; s->hp_prev = nz;
        if (s->sn_env > 0.0005f) {
            float sn = (nz * 0.7f + 0.3f * sinf(2 * PI_F * 185.f * s->sn_env * 8)) * s->sn_env * 0.32f;
            s->sn_env *= s->style == 1 ? 0.99955f : 0.9994f;
            l += sn; r += sn; send += sn * 0.3f;
        }
        if (s->hat_env > 0.0005f) { float h = hp * s->hat_env * 0.12f; s->hat_env *= 0.9975f; l += h * 0.8f; r += h; }
        if (s->ohat_env > 0.0005f) { float h = hp * s->ohat_env * 0.1f; s->ohat_env *= 0.9993f; l += h; r += h * 0.8f; }
        if (s->style == 1) { float c = (s->noise & 0xFFF) == 7 ? 0.25f : 0; l += c + nz * 0.004f; r += c + nz * 0.004f; }
        /* reverb */
        float rv = 0;
        for (int c = 0; c < 4; c++) {
            int p = s->comb_pos[c];
            float o = s->comb[c][p];
            s->comb[c][p] = send + o * s->comb_fb[c];
            s->comb_pos[c] = (p + 1) % COMB_LEN[c];
            rv += o;
        }
        for (int a = 0; a < 2; a++) {
            int p = s->ap_pos[a];
            float b = s->ap[a][p];
            float o = -rv + b;
            s->ap[a][p] = rv + b * 0.5f;
            s->ap_pos[a] = (p + 1) % AP_LEN[a];
            rv = o;
        }
        rv *= 0.18f;
        /* stereo delay (dotted eighth) */
        int dlen = (int)(sps * 3) % SR;
        int rp = (s->dl_pos - dlen + SR) % SR;
        float dl = s->dl_l[rp], dr = s->dl_r[rp];
        s->dl_l[s->dl_pos] = send * 0.4f + dr * 0.42f;
        s->dl_r[s->dl_pos] = dl * 0.42f;
        s->dl_pos = (s->dl_pos + 1) % SR;
        l += rv + dl * 0.5f; r += rv * 0.9f + dr * 0.5f;
        if (s->style == 1) { /* lo-fi tone */
            s->master_lp_l += 0.35f * (l - s->master_lp_l); s->master_lp_r += 0.35f * (r - s->master_lp_r);
            l = s->master_lp_l; r = s->master_lp_r;
        }
        /* fade in / out over the track */
        float t = s->sample / (float)SR;
        float fade = fminf(1.f, t / 1.5f) * fminf(1.f, (TRACK_SECONDS - t) / 4.f);
        out[2 * i] = tanhf(l * 0.9f) * fade;
        out[2 * i + 1] = tanhf(r * 0.9f) * fade;
        s->sample++;
        s->beat *= 0.99993f;
    }
}

/* ---------------- Output & clock ---------------- */
static int find_exe(const char *name) {
    const char *path = getenv("PATH");
    if (!path) return 0;
    char buf[1024];
    const char *p = path;
    while (*p) {
        const char *e = strchr(p, ':');
        int len = e ? (int)(e - p) : (int)strlen(p);
        snprintf(buf, sizeof buf, "%.*s/%s", len, p, name);
        if (access(buf, X_OK) == 0) return 1;
        if (!e) break;
        p = e + 1;
    }
    return 0;
}

static void *audio_thread(void *arg) {
    (void)arg;
    float block[512 * 2];
    int16_t pcm[512 * 2];
    A.last = now_sec();
    while (A.running) {
        double t = now_sec();
        double dt = t - A.last;
        A.last = t;
        pthread_mutex_lock(&A.m);
        if (A.playing && !A.paused && !A.manual) A.pos += dt;
        if (A.playing && A.pos >= TRACK_SECONDS) {
            A.track = (A.track + 1) % 5;
            A.dur = audio_track_length(A.album, A.track);
            A.pos = 0; A.gen = 0;
            synth_setup(&A.syn, A.album, A.track);
        }
        int64_t target = (int64_t)((A.pos + 0.12) * SR);
        int produced = 0;
        while (A.playing && A.gen < target && produced < 8) {
            synth_render(&A.syn, block, 512);
            for (int i = 0; i < 512; i++) {
                int64_t idx = (A.gen + i) & (RING - 1);
                A.ring[idx][0] = block[2 * i];
                A.ring[idx][1] = block[2 * i + 1];
            }
            A.gen += 512;
            produced++;
            if (A.out) {
                for (int i = 0; i < 1024; i++) pcm[i] = (int16_t)(clampf(block[i] * (A.paused ? 0 : 1), -1, 1) * 32000.f * 0.8f);
                pthread_mutex_unlock(&A.m);
                fwrite(pcm, 2, 1024, A.out);
                pthread_mutex_lock(&A.m);
            }
        }
        /* Keep the device fed with UI sounds / silence when idle. */
        if (A.out && (!A.playing || A.paused)) {
            for (int i = 0; i < 512; i++) {
                float v = 0;
                if (A.sfx_req) { A.sfx_env = 1; A.sfx_ph = 0; A.sfx_f = A.sfx_req == 1 ? 1400.f : 880.f; A.sfx_kind = A.sfx_req; A.sfx_req = 0; }
                if (A.sfx_env > 0.001f) { A.sfx_ph += A.sfx_f / SR; v = sinf(2 * PI_F * A.sfx_ph) * A.sfx_env * 0.12f; A.sfx_env *= 0.9985f; }
                pcm[2 * i] = pcm[2 * i + 1] = (int16_t)(v * 32000);
            }
            pthread_mutex_unlock(&A.m);
            fwrite(pcm, 2, 1024, A.out);
            continue;
        }
        pthread_mutex_unlock(&A.m);
        usleep(4000);
    }
    return NULL;
}

void audio_init(int enable_output) {
    pthread_mutex_init(&A.m, NULL);
    snprintf(A.dev, sizeof A.dev, "none (silent clock)");
    if (enable_output) {
        const char *cmd = getenv("KEEL_AUDIO_CMD");
        if (!cmd) {
            if (find_exe("pw-cat")) cmd = "pw-cat --playback --format s16 --rate 44100 --channels 2 - 2>/dev/null";
            else if (find_exe("pacat")) cmd = "pacat --format=s16le --rate=44100 --channels=2 2>/dev/null";
            else if (find_exe("aplay")) cmd = "aplay -q -f S16_LE -r 44100 -c 2 2>/dev/null";
        }
        if (cmd) {
            A.out = popen(cmd, "w");
            if (A.out) snprintf(A.dev, sizeof A.dev, "%.20s", cmd);
        }
    }
    A.album = -1;
    A.running = 1;
    pthread_create(&A.th, NULL, audio_thread, NULL);
}

void audio_shutdown(void) {
    A.running = 0;
    pthread_join(A.th, NULL);
    if (A.out) pclose(A.out);
}

void audio_play(int album, int track) {
    pthread_mutex_lock(&A.m);
    A.album = album; A.track = track;
    A.dur = audio_track_length(album, track);
    A.playing = 1; A.paused = 0;
    A.pos = 0; A.gen = 0;
    synth_setup(&A.syn, album, track);
    memset(A.ring, 0, sizeof A.ring);
    pthread_mutex_unlock(&A.m);
}
void audio_stop(void) { pthread_mutex_lock(&A.m); A.playing = 0; pthread_mutex_unlock(&A.m); }
void audio_toggle_pause(void) { pthread_mutex_lock(&A.m); if (A.playing) A.paused = !A.paused; pthread_mutex_unlock(&A.m); }
void audio_next(void) { if (A.album >= 0) audio_play(A.album, (A.track + 1) % 5); }
void audio_prev(void) {
    if (A.album < 0) return;
    if (A.pos > 3) audio_play(A.album, A.track);
    else audio_play(A.album, (A.track + 4) % 5);
}
void audio_seek(float seconds) {
    pthread_mutex_lock(&A.m);
    double p = clampf((float)(A.pos + seconds), 0, TRACK_SECONDS - 1);
    /* The synth is a forward-running sequencer: render ahead quickly to seek. */
    if (p < A.pos) { synth_setup(&A.syn, A.album, A.track); A.gen = 0; }
    float tmp[1024 * 2];
    while (A.gen < (int64_t)(p * SR)) { synth_render(&A.syn, tmp, 1024); A.gen += 1024; }
    A.pos = p;
    pthread_mutex_unlock(&A.m);
}
int audio_playing(void) { return A.playing; }
int audio_paused(void) { return A.paused; }
int audio_album(void) { return A.album; }
int audio_track(void) { return A.track; }
float audio_position(void) { return (float)A.pos; }
float audio_duration(void) { return TRACK_SECONDS; }
const char *audio_device(void) { return A.dev; }
void sfx_play(int id) { A.sfx_req = id == SFX_MOVE ? 1 : 2; }
void audio_set_manual_clock(int on) { A.manual = on; }
void audio_tick(float dt) {
    if (!A.manual) return;
    pthread_mutex_lock(&A.m);
    if (A.playing && !A.paused) A.pos += dt;
    pthread_mutex_unlock(&A.m);
}

/* ---------------- Analysis ---------------- */
#define FFTN 2048
static void fft(float *re, float *im, int n) {
    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) { float t = re[i]; re[i] = re[j]; re[j] = t; t = im[i]; im[i] = im[j]; im[j] = t; }
    }
    for (int len = 2; len <= n; len <<= 1) {
        float ang = -2 * PI_F / len, wr = cosf(ang), wi = sinf(ang);
        for (int i = 0; i < n; i += len) {
            float cr = 1, ci = 0;
            for (int j = 0; j < len / 2; j++) {
                float ur = re[i + j], ui = im[i + j];
                float vr = re[i + j + len / 2] * cr - im[i + j + len / 2] * ci;
                float vi = re[i + j + len / 2] * ci + im[i + j + len / 2] * cr;
                re[i + j] = ur + vr; im[i + j] = ui + vi;
                re[i + j + len / 2] = ur - vr; im[i + j + len / 2] = ui - vi;
                float nr = cr * wr - ci * wi;
                ci = cr * wi + ci * wr; cr = nr;
            }
        }
    }
}

static void grab(float *mono, int n) {
    pthread_mutex_lock(&A.m);
    int64_t end = (int64_t)(A.pos * SR);
    if (end > A.gen) end = A.gen;
    for (int i = 0; i < n; i++) {
        int64_t idx = end - n + i;
        if (idx < 0 || !A.playing) { mono[i] = 0; continue; }
        idx &= RING - 1;
        mono[i] = (A.ring[idx][0] + A.ring[idx][1]) * 0.5f;
    }
    pthread_mutex_unlock(&A.m);
}

void audio_spectrum(float *bins, int n) {
    static float re[FFTN], im[FFTN];
    grab(re, FFTN);
    for (int i = 0; i < FFTN; i++) { float w = 0.5f - 0.5f * cosf(2 * PI_F * i / (FFTN - 1)); re[i] *= w; im[i] = 0; }
    fft(re, im, FFTN);
    if (n > 128) n = 128;
    for (int b = 0; b < n; b++) {
        float f0 = 35.f * powf(16000.f / 35.f, (float)b / n), f1 = 35.f * powf(16000.f / 35.f, (float)(b + 1) / n);
        int k0 = (int)(f0 * FFTN / SR), k1 = MAXI(k0 + 1, (int)(f1 * FFTN / SR));
        float m = 0;
        for (int k = k0; k < k1 && k < FFTN / 2; k++) m = fmaxf(m, sqrtf(re[k] * re[k] + im[k] * im[k]));
        float db = 20.f * log10f(m / (FFTN * 0.25f) + 1e-6f);
        float v = clampf((db + 62.f) / 54.f, 0, 1);
        v *= 0.75f + 0.5f * b / n; /* tilt so highs are visible */
        float cur = A.spec[b];
        A.spec[b] = v > cur ? cur + (v - cur) * 0.6f : cur * 0.88f + v * 0.12f;
        bins[b] = clampf(A.spec[b], 0, 1);
    }
}

void audio_waveform(float *out, int n) {
    static float buf[4096];
    if (n > 4096) n = 4096;
    grab(buf, n);
    for (int i = 0; i < n; i++) out[i] = buf[i];
}

float audio_level(void) {
    float buf[1024];
    grab(buf, 1024);
    float s = 0;
    for (int i = 0; i < 1024; i++) s += buf[i] * buf[i];
    return sqrtf(s / 1024);
}

float audio_beat(void) {
    /* Kick detection from low-band energy of the heard signal. */
    float buf[512];
    grab(buf, 512);
    float lp = 0, e = 0;
    for (int i = 0; i < 512; i++) { lp += 0.02f * (buf[i] - lp); e += lp * lp; }
    e = sqrtf(e / 512) * 6.f;
    A.beat_vis = fmaxf(A.beat_vis * 0.86f, clampf(e, 0, 1));
    return A.beat_vis;
}
