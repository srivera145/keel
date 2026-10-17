/* KEEL OS — the "movie" you play is rendered live: the scene's four depth
 * layers are panned by a virtual camera with parallax, cut between shots,
 * and dressed with weather/particles, lens flare and film grain. */
#include "film.h"
#include "app.h"
#include "assets.h"
#include "jobs.h"
#include <stdatomic.h>

#define LW 2304
#define LH 1080
#define SLOTS 2

typedef struct { int movie; Texture *layers[4]; atomic_int state; /* 0 empty 1 busy 2 ready */ uint64_t used; } FilmSlot;
static FilmSlot slots[SLOTS] = {{-1}, {-1}};
static uint64_t use_clock;

static void gen_job(void *arg) {
    FilmSlot *s = arg;
    Movie *m = &g_movies[s->movie];
    for (int l = 0; l < 4; l++)
        if (!s->layers[l]) s->layers[l] = tex_new(LW, LH);
    scene_render_layers(s->layers, &m->scene, (float)LW / LH);
    atomic_store(&s->state, 2);
}

static FilmSlot *find(int movie) {
    for (int i = 0; i < SLOTS; i++)
        if (slots[i].movie == movie) return &slots[i];
    return NULL;
}

void film_prepare(int movie) {
    FilmSlot *s = find(movie);
    if (s) { s->used = ++use_clock; return; }
    /* reuse the least recently used idle slot */
    FilmSlot *best = NULL;
    for (int i = 0; i < SLOTS; i++) {
        if (atomic_load(&slots[i].state) == 1) continue;
        if (!best || slots[i].used < best->used) best = &slots[i];
    }
    if (!best) return;
    best->movie = movie;
    best->used = ++use_clock;
    atomic_store(&best->state, 1);
    job_submit(gen_job, best, 1000);
}

int film_ready(int movie) {
    FilmSlot *s = find(movie);
    return s && atomic_load(&s->state) == 2;
}

static float hashrf(uint32_t i, uint32_t k) { return hashf(i * 2654435761u + k * 97u); }

static void particles(Gfx *g, const SceneDesc *d, double t, float sx, float sy, float k) {
    int kind = d->kind;
    int night = d->tod == TOD_NIGHT || d->tod == TOD_MAGIC;
    if (kind == SC_AURORA || (kind == SC_MOUNTAINS && d->tod == TOD_DAY)) {
        /* snow */
        for (int i = 0; i < 160; i++) {
            float sp = 40 + hashrf(i, 1) * 80, z = 0.4f + hashrf(i, 2) * 0.9f;
            float x = fmodf(hashrf(i, 3) * 2200 + (float)t * 25 * z + sinf((float)t * 0.8f + i) * 20, 2200) - 140;
            float y = fmodf(hashrf(i, 4) * 1200 + (float)t * sp * z, 1200) - 60;
            g_circle(g, sx + x * k, sy + y * k, (1.2f + 2.2f * z) * k, rgba(255, 255, 255, (int)(110 + 100 * z)));
        }
    } else if (kind == SC_DESERT || kind == SC_CANYON) {
        /* drifting dust / embers */
        for (int i = 0; i < 90; i++) {
            float z = 0.3f + hashrf(i, 2);
            float x = fmodf(hashrf(i, 3) * 2200 + (float)t * 60 * z, 2200) - 140;
            float y = 500 + hashrf(i, 4) * 520 + sinf((float)t * 0.7f + i) * 30;
            g_circle(g, sx + x * k, sy + y * k, (1.f + 1.6f * z) * k, rgba(255, 220, 170, (int)(70 * z)));
        }
    } else if (kind == SC_FOREST || kind == SC_LAKE || kind == SC_PAPER) {
        /* fireflies / pollen */
        for (int i = 0; i < 50; i++) {
            float x = hashrf(i, 3) * 1920 + sinf((float)t * 0.4f + i * 1.7f) * 60;
            float y = 450 + hashrf(i, 4) * 600 + cosf((float)t * 0.5f + i) * 40;
            float a = 0.5f + 0.5f * sinf((float)t * 2.f + i * 3.1f);
            g_glow(g, sx + x * k, sy + y * k, 10 * k, rgba(255, 230, 150, (int)(150 * a)));
        }
    } else if (kind == SC_CITY) {
        if (night) {
            /* rain */
            for (int i = 0; i < 180; i++) {
                float z = 0.4f + hashrf(i, 2) * 0.8f;
                float x = fmodf(hashrf(i, 3) * 2100 - (float)t * 120 * z, 2100);
                float y = fmodf(hashrf(i, 4) * 1200 + (float)t * 900 * z, 1200) - 60;
                g_line(g, sx + x * k, sy + y * k, sx + (x - 6 * z) * k, sy + (y + 28 * z) * k, 1.4f * k, rgba(200, 220, 255, (int)(70 * z)));
            }
        }
    } else if (kind == SC_SPACE || kind == SC_SYNTH) {
        for (int i = 0; i < 70; i++) {
            float a = 0.5f + 0.5f * sinf((float)t * (1.5f + hashrf(i, 5) * 3) + i);
            float x = hashrf(i, 3) * 1920, y = hashrf(i, 4) * 600;
            g_circle(g, sx + x * k, sy + y * k, 1.6f * k, rgba(255, 255, 255, (int)(200 * a * a)));
        }
    } else if (kind == SC_OCEAN) {
        /* gulls drifting */
        for (int i = 0; i < 6; i++) {
            float x = fmodf(hashrf(i, 3) * 2000 + (float)t * (30 + 20 * hashrf(i, 2)), 2200) - 140;
            float y = 220 + hashrf(i, 4) * 200 + sinf((float)t * 1.3f + i) * 12;
            float fl = sinf((float)t * 7 + i * 2) * 6;
            g_line(g, sx + (x - 12) * k, sy + (y - fl) * k, sx + x * k, sy + y * k, 2.2f * k, rgba(40, 30, 50, 200));
            g_line(g, sx + x * k, sy + y * k, sx + (x + 12) * k, sy + (y - fl) * k, 2.2f * k, rgba(40, 30, 50, 200));
        }
    }
}

void film_draw(Gfx *g, int movie, double t, float x, float y, float w, float h, int letterbox) {
    const Movie *m = &g_movies[movie];
    FilmSlot *s = find(movie);
    float k = w / 1920.f;
    g_push(g);
    g_clip(g, x, y, w, h);
    g_rect(g, x, y, w, h, 0, BLACK);
    /* shots: pan right, pan left with push-in, slow push-in */
    const float shot_len = 14.f;
    int shot = (int)(t / shot_len);
    float st = (float)fmod(t, shot_len) / shot_len;
    float cam, zoom;
    switch (shot % 3) {
    case 0: cam = lerpf(0.15f, 0.85f, ease_in_out_cubic(st)); zoom = 1.0f; break;
    case 1: cam = lerpf(0.9f, 0.2f, ease_in_out_cubic(st)); zoom = 1.0f + 0.06f * st; break;
    default: cam = 0.5f + 0.1f * sinf(st * 3.f); zoom = 1.04f + 0.08f * st; break;
    }
    if (s && atomic_load(&s->state) == 2) {
        static const float par[4] = {0.18f, 0.42f, 0.7f, 1.0f};
        const float range = LW - 1920;
        for (int l = 0; l < 4; l++) {
            float z = 1.f + (zoom - 1.f) * (0.4f + 0.6f * l / 3.f);
            float sw = 1920 / z, sh = 1080 / z;
            float off = (range * cam * par[l]) + range * 0.5f * (1 - par[l]);
            float bob = sinf((float)t * 0.37f) * 4 * par[l];
            g_image_src(g, s->layers[l], off + (1920 - sw) * 0.5f, (1080 - sh) * 0.5f + bob, sw, sh, x, y, w, h, 0, 1);
        }
    } else {
        /* still generating: Ken Burns on the backdrop */
        const Texture *b = m->backdrop && m->backdrop->ready ? m->backdrop : m->blur;
        if (b && b->ready) {
            float z = 1.05f + 0.05f * st;
            float sw = b->w / z, sh = b->h / z;
            g_image_src(g, b, (b->w - sw) * cam, (b->h - sh) * 0.5f, sw, sh, x, y, w, h, 0, 1);
        }
    }
    particles(g, &m->scene, t, x, y, k);
    /* sun flare */
    /* the sun sits at the center column of the sky layer */
    float off0 = (LW - 1920) * (cam * 0.18f + 0.5f * 0.82f);
    float fx = x + (LW * 0.5f - off0) * k;
    float fy = y + m->scene.sun_y * 1080 * k;
    if (m->scene.kind != SC_SPACE && m->scene.tod != TOD_NIGHT) {
        float pulse = 0.85f + 0.15f * sinf((float)t * 0.9f);
        g_glow(g, fx, fy, 240 * k, rgba(255, 230, 190, (int)(50 * pulse)));
        float cx = x + w / 2, cy = y + h / 2;
        for (int i = 1; i <= 3; i++) {
            float px = fx + (cx - fx) * (0.6f + i * 0.45f), py = fy + (cy - fy) * (0.6f + i * 0.45f);
            g_circle(g, px, py, (14 + i * 10) * k, rgba(255, 220, 200, 14 + i * 4));
        }
    }
    /* scene cut flash */
    float cut = (float)fmod(t, shot_len);
    if (cut < 0.35f && t > 1) g_rect(g, x, y, w, h, 0, rgba(0, 0, 0, (int)(255 * (1 - cut / 0.35f))));
    /* vignette */
    g_rect_hgrad(g, x, y, w * 0.2f, h, 0, rgba(0, 0, 0, 90), rgba(0, 0, 0, 0));
    g_rect_hgrad(g, x + w * 0.8f, y, w * 0.2f, h, 0, rgba(0, 0, 0, 0), rgba(0, 0, 0, 90));
    if (letterbox) {
        float bar = (h - w / 2.39f) * 0.5f;
        g_rect(g, x, y, w, bar, 0, BLACK);
        g_rect(g, x, y + h - bar, w, bar, 0, BLACK);
    }
    g_pop(g);
}
