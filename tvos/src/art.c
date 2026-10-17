/* KEEL OS — procedural art: cinematic landscapes, posters, photos, album
 * covers, wallpaper, channel logos. No stock assets: every pixel is computed.
 *
 * Scenes live in a world space one unit tall (y down, horizon around 0.6)
 * and ~1.78 units wide; a texture is a window onto that world. Scenes are
 * built from four depth layers (sky, far, mid, near) so the player can pan
 * them with parallax. */
#include "assets.h"
#include "font.h"
#include "icons.h"
#include "jobs.h"
#include "raster.h"
#include <stdatomic.h>

typedef struct { uint32_t z, m, h, sun, haze, far, near, water; float stars, clouds; } PalDef;
static const PalDef PALS[TOD_COUNT] = {
    [TOD_DAWN] = {0x2E3A8C, 0xC77DCB, 0xFFC7A6, 0xFFF2CC, 0xF6B8C8, 0x9A7BB5, 0x2B2350, 0x6D78B8, 0.15f, 0.55f},
    [TOD_GOLDEN] = {0x2E6CCB, 0xF4A65F, 0xFFE3A1, 0xFFF7DA, 0xF8C98A, 0xC07D5E, 0x3A2216, 0x5A7FB5, 0.0f, 0.6f},
    [TOD_DUSK] = {0x1D1B52, 0x8A2F8F, 0xFF7E5F, 0xFFD6A0, 0xE8707F, 0x6A3478, 0x1A1030, 0x4A3B7A, 0.35f, 0.45f},
    [TOD_NIGHT] = {0x040A1E, 0x0E2250, 0x1F5480, 0xE9F2FF, 0x2B4F80, 0x172C52, 0x050A18, 0x123058, 1.0f, 0.12f},
    [TOD_DAY] = {0x2F7EF5, 0x7DB7FF, 0xE3F1FF, 0xFFFFFF, 0xD2E6F8, 0x7D9EC4, 0x27443A, 0x2F6FB5, 0.0f, 0.85f},
    [TOD_MAGIC] = {0x12305E, 0x5D4FA0, 0xF7A6C8, 0xFFEAF4, 0xC9A6DC, 0x6B5EA3, 0x231A40, 0x3E4E8E, 0.5f, 0.35f},
};

typedef struct {
    C3 z, m, h, sun, haze, far, near, water;
    float stars, clouds;
} Pal;

typedef struct {
    Surface s;
    Gfx *g;
    int W, H;
    float sc, wx0, hy;
    Pal p;
    const SceneDesc *d;
    uint32_t seed;
} SR;

static inline float PXr(SR *r, float wx) { return (wx - r->wx0) * r->sc; }
static inline float PYr(SR *r, float wy) { return wy * r->sc; }
static inline float WXr(SR *r, float px) { return r->wx0 + px / r->sc; }

static Pal make_pal(const SceneDesc *d) {
    const PalDef *pd = &PALS[d->tod];
    Pal p = {c3hex(pd->z), c3hex(pd->m), c3hex(pd->h), c3hex(pd->sun), c3hex(pd->haze), c3hex(pd->far), c3hex(pd->near), c3hex(pd->water), pd->stars, pd->clouds};
    switch (d->kind) {
    case SC_SYNTH:
        p.z = c3hex(0x12052E); p.m = c3hex(0x6A1B9A); p.h = c3hex(0xFF5FA2); p.sun = c3hex(0xFFD36E);
        p.haze = c3hex(0xFF7AC0); p.far = c3hex(0x2A0F4F); p.near = c3hex(0x0C0420); p.stars = 0.6f; p.clouds = 0; break;
    case SC_SPACE: p.clouds = 0; p.stars = 1.0f;
        p.z = c3scale(p.z, 0.5f); p.m = c3scale(p.m, 0.6f); p.h = c3lerp(p.m, p.h, 0.5f); break;
    case SC_AURORA: p.clouds = 0.05f; if (p.stars < 0.6f) p.stars = 0.6f;
        if (d->tod != TOD_NIGHT) { p.z = c3hex(0x0A1838); p.m = c3hex(0x1C3A6E); p.h = c3hex(0x6E7FB8); p.far = c3hex(0x3B4F80); } break;
    case SC_PAPER: p.clouds = 0; p.stars = 0;
        p.z = c3lerp(p.z, c3(1, 1, 1), 0.35f); p.m = c3lerp(p.m, c3(1, 1, 1), 0.3f); p.h = c3lerp(p.h, c3(1, 1, 1), 0.25f); break;
    case SC_CANYON: p.far = c3lerp(p.far, c3hex(0xC8643B), 0.6f); p.near = c3lerp(p.near, c3hex(0x3B1408), 0.7f); break;
    case SC_DESERT: p.far = c3lerp(p.far, c3hex(0xE0A060), 0.5f); break;
    case SC_FOREST: p.far = c3lerp(p.far, c3hex(0x4E7A6A), 0.5f); p.near = c3lerp(p.near, c3hex(0x0E2018), 0.6f); break;
    }
    return p;
}

uint32_t scene_accent(const SceneDesc *d) {
    Pal p = make_pal(d);
    C3 c = c3lerp(p.h, p.sun, 0.3f);
    if (d->tod == TOD_NIGHT) c = c3lerp(p.h, c3(0.5f, 0.8f, 1.f), 0.5f);
    return c3col(c, 1);
}
uint32_t scene_shadow(const SceneDesc *d) {
    Pal p = make_pal(d);
    return c3col(c3lerp(p.near, p.z, 0.4f), 1);
}

static float horizon_of(const SceneDesc *d) {
    switch (d->kind) {
    case SC_OCEAN: return 0.6f;
    case SC_CITY: return 0.7f;
    case SC_SYNTH: return 0.6f;
    case SC_LAKE: return 0.58f;
    case SC_SPACE: return 0.8f;
    default: return 0.64f;
    }
}

/* ---------------- low-res field helper ---------------- */
typedef struct { float *v; int w, h, ch; float sx, sy; } Field;
static Field field_new(int W, int H, int ch, int div) {
    Field f;
    f.w = W / div + 2; f.h = H / div + 2; f.ch = ch;
    f.sx = (float)(f.w - 2) / W; f.sy = (float)(f.h - 2) / H;
    f.v = xcalloc((size_t)f.w * f.h * ch, sizeof(float));
    return f;
}
static inline void field_sample(const Field *f, float x, float y, float *out) {
    float fx = x * f->sx, fy = y * f->sy;
    int ix = (int)fx, iy = (int)fy;
    float tx = fx - ix, ty = fy - iy;
    if (ix >= f->w - 1) { ix = f->w - 2; tx = 1; }
    if (iy >= f->h - 1) { iy = f->h - 2; ty = 1; }
    const float *a = f->v + ((size_t)iy * f->w + ix) * f->ch, *b = a + f->ch, *c = a + (size_t)f->w * f->ch, *d = c + f->ch;
    for (int k = 0; k < f->ch; k++) out[k] = lerpf(lerpf(a[k], b[k], tx), lerpf(c[k], d[k], tx), ty);
}

/* ---------------- Sky ---------------- */
static C3 sky_grad(SR *r, float wy) {
    float t = clampf(wy / r->hy, 0, 1);
    if (t < 0.55f) return c3lerp(r->p.z, r->p.m, powf(t / 0.55f, 1.15f));
    return c3lerp(r->p.m, r->p.h, smoothstep(0, 1, (t - 0.55f) / 0.45f));
}

static void sky_layer(SR *r) {
    const SceneDesc *d = r->d;
    int W = r->W, H = r->H;
    uint32_t seed = r->seed;
    float sunx = d->sun_x, suny = d->sun_y;
    float glow_amt = d->tod == TOD_NIGHT ? 0.25f : (d->tod == TOD_DAY ? 0.5f : 0.85f);
    if (d->kind == SC_SPACE) glow_amt = 0.2f;
    /* Clouds / nebula fields at quarter resolution. */
    Field cl = field_new(W, H, 3, 4);
    int nebula = d->kind == SC_SPACE || (d->kind == SC_AURORA && d->photo == 0 && 0);
    C3 n1 = c3lerp(r->p.h, c3(1.f, 0.3f, 0.6f), 0.4f), n2 = c3lerp(r->p.m, c3(0.2f, 0.6f, 1.f), 0.5f);
    for (int j = 0; j < cl.h; j++)
        for (int i = 0; i < cl.w; i++) {
            float px = i / cl.sx, py = j / cl.sy;
            float wx = WXr(r, px), wy = py / r->sc;
            float *o = cl.v + ((size_t)j * cl.w + i) * 3;
            if (nebula) {
                float a = fbm2(wx * 1.6f, wy * 1.6f, 6, seed + 11);
                float b = fbm2(wx * 2.4f + 7, wy * 2.4f, 5, seed + 29);
                float dust = fbm2(wx * 3.f + 3, wy * 3.f, 5, seed + 41);
                float band = expf(-powf((wy - 0.35f - 0.15f * sinf(wx * 1.3f + seed)) * 3.f, 2));
                o[0] = smoothstep(0.42f, 0.8f, a) * (0.4f + 0.8f * band);
                o[1] = smoothstep(0.48f, 0.85f, b) * (0.3f + 0.7f * band);
                o[2] = smoothstep(0.5f, 0.75f, dust) * band;
            } else if (r->p.clouds > 0) {
                float hfade = 1.f - smoothstep(r->hy - 0.12f, r->hy - 0.01f, wy);
                float stretch = 2.2f + (d->tod == TOD_DAY ? 0 : 1.5f);
                float n = fbm2(wx * 2.2f, wy * 2.2f * stretch, 6, seed + 3);
                float c = smoothstep(0.5f, 0.78f, n) * hfade * r->p.clouds;
                float n2 = fbm2(wx * 2.2f, (wy - 0.015f) * 2.2f * stretch, 6, seed + 3);
                o[0] = c;
                o[1] = clampf((n - n2) * 6.f, -1, 1); /* lit from above */
                o[2] = 0;
            }
        }
    for (int y = 0; y < H; y++) {
        uint32_t *row = r->s.px + (size_t)y * r->s.stride;
        float wy = y / r->sc;
        C3 base = sky_grad(r, wy);
        for (int x = 0; x < W; x++) {
            float wx = WXr(r, (float)x);
            C3 c = base;
            float dx = wx - sunx, dy = wy - suny;
            float dist = sqrtf(dx * dx + dy * dy * 1.4f);
            float gl = expf(-dist * 3.2f) * 0.55f + expf(-dist * 12.f) * 0.6f;
            c = c3add(c, c3scale(r->p.sun, gl * glow_amt));
            float f[3];
            field_sample(&cl, (float)x, (float)y, f);
            if (nebula) {
                c = c3add(c, c3scale(n1, f[0] * 0.75f));
                c = c3add(c, c3scale(n2, f[1] * 0.6f));
                c = c3scale(c, 1.f - f[2] * 0.45f);
            } else if (f[0] > 0.001f) {
                float lit = 0.5f + 0.5f * f[1];
                float sunp = expf(-dist * 2.5f);
                C3 cc = c3lerp(c3lerp(r->p.haze, r->p.m, 0.25f), c3lerp(r->p.sun, c3(1, 1, 1), 0.3f), clampf(lit * 0.7f + sunp * 0.6f, 0, 1));
                if (d->tod == TOD_NIGHT) cc = c3lerp(r->p.m, r->p.h, lit);
                c = c3lerp(c, cc, clampf(f[0] * 1.1f, 0, 0.92f));
            }
            if (d->kind == SC_AURORA) {
                for (int k = 0; k < 2; k++) {
                    float u = wx + k * 0.7f;
                    float center = 0.2f + k * 0.12f + 0.07f * sinf(u * 2.3f + seed % 7 + k) + 0.05f * (fbm1(u * 2.5f, 3, seed + 9 + k) - 0.5f);
                    float ddy = wy - center;
                    float band = ddy > 0 ? expf(-ddy * ddy / 0.0012f) : expf(-ddy * ddy / (0.02f + 0.01f * k));
                    float streak = 0.35f + 0.65f * fbm1(u * 70.f, 2, seed + 21 + k);
                    float inten = band * streak * (0.55f + 0.45f * sinf(u * 3.1f + k * 2.f)) * (k ? 0.6f : 1.f);
                    C3 ac = c3lerp(c3(0.25f, 1.f, 0.6f), c3(0.3f, 0.6f, 1.f), clampf(-ddy * 8.f, 0, 1));
                    ac = c3lerp(ac, c3(0.9f, 0.3f, 0.9f), clampf(-ddy * 4.f - 0.3f, 0, 1) * (k ? 1.f : 0.5f));
                    c = c3add(c, c3scale(ac, inten * 0.85f));
                }
            }
            if (r->p.stars > 0 && wy < r->hy) {
                uint32_t h = hash2i((int)(wx * 1000.f), y * 1080 / H, seed);
                float st = (h & 0xFFFF) / 65535.f;
                float thr = 0.0035f * r->p.stars;
                if (st < thr) {
                    float b = ((h >> 16) & 255) / 255.f;
                    float fade = 1.f - smoothstep(r->hy * 0.4f, r->hy, wy);
                    c = c3add(c, c3scale(c3(0.9f, 0.95f, 1.f), b * b * fade * (0.6f + 0.4f * r->p.stars)));
                }
            }
            row[x] = c3col(c, 1);
        }
    }
    free(cl.v);
    /* Bright stars with a soft glow */
    if (r->p.stars > 0.3f) {
        Rng rg = rng_make(seed + 77);
        int n = (int)(40 * r->p.stars * r->W / 1920.f + 6);
        for (int i = 0; i < n; i++) {
            float x = rng_f(&rg) * W, y = rng_f(&rg) * r->hy * 0.8f * r->sc;
            float s = rng_range(&rg, 1.f, 2.4f) * r->sc / 1080.f;
            g_glow(r->g, x, y, s * 7, rgba(200, 220, 255, 60));
            g_circle(r->g, x, y, fmaxf(0.7f, s), rgba(255, 255, 255, 230));
        }
    }
    /* Sun / moon */
    float sx = PXr(r, sunx), sy = PYr(r, suny);
    if (d->kind == SC_SYNTH) {
        float R = 0.2f * r->sc;
        Path p; path_init(&p);
        path_circle(&p, sx, sy, R);
        Paint pt = {PAINT_LINEAR, c3col(c3hex(0xFFE66D), 1), c3col(c3hex(0xFF3F9E), 1), sx, sy - R, sx, sy + R, 0};
        g_glow(r->g, sx, sy, R * 2.2f, rgba(255, 90, 160, 120));
        path_fill(&r->s, &p, &pt);
        path_free(&p);
        for (int i = 0; i < 7; i++) {
            float yy = sy + R * (0.1f + i * 0.13f), hh = R * (0.02f + i * 0.012f);
            C3 bc = sky_grad(r, (yy + hh * 0.5f) / r->sc);
            g_rect(r->g, sx - R - 2, yy, 2 * R + 4, hh, 0, c3col(bc, 1));
        }
    } else if (d->kind == SC_PAPER) {
        for (int i = 3; i >= 0; i--) {
            float R = (0.06f + i * 0.035f) * r->sc;
            g_shadow(r->g, sx - R, sy - R + 4, 2 * R, 2 * R, R, 14, rgba(80, 40, 60, 40));
            g_circle(r->g, sx, sy, R, c3col(c3lerp(r->p.sun, r->p.h, i * 0.22f), 1));
        }
    } else if (d->kind != SC_SPACE) {
        int night = d->tod == TOD_NIGHT || d->kind == SC_AURORA;
        float R = (night ? 0.03f : (d->tod == TOD_DAY ? 0.035f : 0.065f)) * r->sc;
        g_glow(r->g, sx, sy, R * 4.f, c3col(r->p.sun, night ? 0.25f : 0.55f));
        g_circle(r->g, sx, sy, R, c3col(c3lerp(r->p.sun, c3(1, 1, 1), 0.5f), 1));
        if (night) {
            g_circle(r->g, sx - R * 0.3f, sy - R * 0.2f, R * 0.22f, rgba(200, 210, 230, 60));
            g_circle(r->g, sx + R * 0.35f, sy + R * 0.25f, R * 0.15f, rgba(200, 210, 230, 50));
        }
    }
}

/* ---------------- Terrain helpers ---------------- */
static float ridge_y(float wx, float base, float amp, float freq, int oct, uint32_t seed, float sharp) {
    float n = fbm1(wx * freq + seed % 1000, oct, seed);
    float rn = 1.f - fabsf(2.f * n - 1.f);
    n = lerpf(n, rn, sharp);
    return base - amp * n;
}

typedef struct { float base, amp, freq, sharp; int oct; uint32_t seed; } RidgeP;

static void ridge(SR *r, RidgeP rp, C3 top, C3 bot, float alpha, float rim, float snow) {
    int step = MAXI(2, r->W / 480);
    int n = r->W / step + 3;
    float *pts = xmalloc(sizeof(float) * 2 * (n + 2));
    float miny = 1e9f;
    for (int i = 0; i < n; i++) {
        float px = (float)(i * step - step);
        float wy = ridge_y(WXr(r, px), rp.base, rp.amp, rp.freq, rp.oct, rp.seed, rp.sharp);
        pts[2 * i] = px; pts[2 * i + 1] = PYr(r, wy);
        miny = fminf(miny, pts[2 * i + 1]);
    }
    pts[2 * n] = (float)r->W + 4; pts[2 * n + 1] = (float)r->H + 4;
    pts[2 * n + 2] = -4; pts[2 * n + 3] = (float)r->H + 4;
    Path p; path_init(&p);
    path_poly(&p, pts, n + 2);
    Paint pt = {PAINT_LINEAR, c3col(top, alpha), c3col(bot, alpha), 0, miny, 0, PYr(r, rp.base + 0.12f), 0};
    path_fill(&r->s, &p, &pt);
    path_free(&p);
    if (snow > 0) {
        /* Snow caps: band below the ridge line on the high parts. */
        Path sp; path_init(&sp);
        float *s2 = xmalloc(sizeof(float) * 4 * n);
        for (int i = 0; i < n; i++) {
            float y = pts[2 * i + 1];
            float hgt = (PYr(r, rp.base) - y) / (rp.amp * r->sc);
            float depth = fmaxf(0, hgt - 0.45f) * snow * r->sc * (0.6f + 0.4f * vnoise1(pts[2 * i] * 0.05f, rp.seed + 5));
            s2[2 * i] = pts[2 * i]; s2[2 * i + 1] = y - 0.5f;
            s2[4 * n - 2 - 2 * i] = pts[2 * i]; s2[4 * n - 1 - 2 * i] = y + depth;
        }
        path_poly(&sp, s2, 2 * n);
        Paint spt = {PAINT_LINEAR, c3col(c3lerp(c3(1, 1, 1), r->p.sun, 0.25f), 0.95f * alpha), c3col(c3lerp(c3(0.8f, 0.85f, 1.f), top, 0.4f), 0.85f * alpha), 0, miny, 0, miny + rp.amp * r->sc * 0.5f, 0};
        path_fill(&r->s, &sp, &spt);
        path_free(&sp);
        free(s2);
    }
    if (rim > 0) {
        Path rp2; path_init(&rp2);
        path_stroke(&rp2, pts, n, fmaxf(1.f, r->sc / 700.f), 0);
        path_fill_color(&r->s, &rp2, c3col(c3lerp(r->p.sun, c3(1, 1, 1), 0.3f), rim * alpha));
        path_free(&rp2);
    }
    free(pts);
}

static void haze_band(SR *r, float wy, float h, C3 c, float a) {
    float y = PYr(r, wy), hh = PYr(r, h);
    g_rect_vgrad(r->g, 0, y - hh, (float)r->W, hh, 0, c3col(c, 0), c3col(c, a));
    g_rect_vgrad(r->g, 0, y, (float)r->W, hh * 0.6f, 0, c3col(c, a), c3col(c, 0));
}

/* ---------------- Elements ---------------- */
static void figure(SR *r, float wx, float wy, float h, C3 col) {
    float x = PXr(r, wx), y = PYr(r, wy), Hh = h * r->sc;
    Path p; path_init(&p);
    path_circle(&p, x, y - Hh * 0.9f, Hh * 0.075f);
    float body[] = {x - Hh * 0.1f, y - Hh * 0.8f, x + Hh * 0.1f, y - Hh * 0.8f, x + Hh * 0.16f, y - Hh * 0.33f, x - Hh * 0.15f, y - Hh * 0.33f};
    path_poly(&p, body, 4);
    path_rrect(&p, x - Hh * 0.035f, y - Hh * 0.84f, Hh * 0.07f, Hh * 0.08f, 0);
    float l1[] = {x - Hh * 0.09f, y - Hh * 0.35f, x - Hh * 0.01f, y - Hh * 0.35f, x - Hh * 0.04f, y, x - Hh * 0.1f, y};
    float l2[] = {x + Hh * 0.01f, y - Hh * 0.35f, x + Hh * 0.09f, y - Hh * 0.35f, x + Hh * 0.11f, y, x + Hh * 0.05f, y};
    path_poly(&p, l1, 4);
    path_poly(&p, l2, 4);
    path_fill_color(&r->s, &p, c3col(col, 1));
    path_free(&p);
    /* rim light */
    g_circle(r->g, x + Hh * 0.03f, y - Hh * 0.92f, Hh * 0.03f, c3col(r->p.sun, 0.25f));
}

static void lone_tree(SR *r, float wx, float wy, float h, C3 col, int paper) {
    float x = PXr(r, wx), y = PYr(r, wy), Hh = h * r->sc;
    if (paper) {
        g_shadow(r->g, x - Hh * 0.3f, y - Hh * 1.0f + 6, Hh * 0.6f, Hh * 0.6f, Hh * 0.3f, 12, rgba(40, 20, 40, 50));
        g_rect(r->g, x - Hh * 0.04f, y - Hh * 0.5f, Hh * 0.08f, Hh * 0.5f, 3, c3col(c3hex(0x8D5A3B), 1));
        g_circle(r->g, x, y - Hh * 0.7f, Hh * 0.3f, c3col(c3hex(0x6CC570), 1));
        g_circle(r->g, x - Hh * 0.08f, y - Hh * 0.78f, Hh * 0.14f, rgba(255, 255, 255, 40));
        return;
    }
    Path p; path_init(&p);
    float tr[] = {x - Hh * 0.04f, y, x + Hh * 0.04f, y, x + Hh * 0.025f, y - Hh * 0.55f, x - Hh * 0.025f, y - Hh * 0.55f};
    path_poly(&p, tr, 4);
    Rng rg = rng_make(r->seed + 3);
    for (int i = 0; i < 9; i++) {
        float a = rng_f(&rg) * 2 * PI_F, d = rng_f(&rg) * Hh * 0.22f;
        path_circle(&p, x + cosf(a) * d * 1.4f, y - Hh * 0.7f + sinf(a) * d * 0.7f, Hh * rng_range(&rg, 0.14f, 0.22f));
    }
    path_fill_color(&r->s, &p, c3col(col, 1));
    path_free(&p);
}

static void pine(Path *p, float x, float y, float h) {
    float w = h * 0.36f;
    for (int t = 0; t < 3; t++) {
        float ty = y - h * (0.12f + t * 0.27f), tw = w * (1.f - t * 0.25f);
        float tri[] = {x, ty - h * 0.38f, x + tw, ty + h * 0.08f, x - tw, ty + h * 0.08f};
        path_poly(p, tri, 3);
    }
    path_rect(p, x - h * 0.03f, y - h * 0.06f, h * 0.06f, h * 0.06f);
}

static void tree_line(SR *r, float base, float amp, float hmin, float hmax, float density, C3 col, float alpha, uint32_t seed) {
    Path p; path_init(&p);
    Rng rg = rng_make(seed);
    float x = -20;
    while (x < r->W + 20) {
        float wx = WXr(r, x);
        float gy = ridge_y(wx, base, amp, 2.f, 3, seed, 0);
        float h = rng_range(&rg, hmin, hmax) * r->sc;
        pine(&p, x, PYr(r, gy) + h * 0.02f, h);
        x += h * rng_range(&rg, 0.25f, 0.6f) / density;
    }
    /* ground under the trees */
    int n = r->W / 8 + 3;
    float *pts = xmalloc(sizeof(float) * 2 * (n + 2));
    for (int i = 0; i < n; i++) {
        float px = (float)(i * 8 - 8);
        pts[2 * i] = px; pts[2 * i + 1] = PYr(r, ridge_y(WXr(r, px), base, amp, 2.f, 3, seed, 0));
    }
    pts[2 * n] = (float)r->W + 8; pts[2 * n + 1] = (float)r->H + 8; pts[2 * n + 2] = -8; pts[2 * n + 3] = (float)r->H + 8;
    path_poly(&p, pts, n + 2);
    free(pts);
    path_fill_color(&r->s, &p, c3col(col, alpha));
    path_free(&p);
}

static void lighthouse(SR *r, float wx, float wy, float h) {
    float x = PXr(r, wx), y = PYr(r, wy), Hh = h * r->sc;
    float bw = Hh * 0.13f, tw = Hh * 0.085f;
    C3 dark = c3lerp(r->p.near, c3(0, 0, 0), 0.2f);
    /* beam */
    float lx = x, ly = y - Hh * 0.93f;
    Path b; path_init(&b);
    float beam[] = {lx, ly - Hh * 0.02f, lx - Hh * 3.f, ly - Hh * 0.25f, lx - Hh * 3.f, ly + Hh * 0.18f, lx, ly + Hh * 0.02f};
    path_poly(&b, beam, 4);
    Paint bp = {PAINT_LINEAR, rgba(255, 245, 200, 110), rgba(255, 245, 200, 0), lx, ly, lx - Hh * 3.f, ly, 0};
    path_fill(&r->s, &b, &bp);
    path_free(&b);
    for (int i = 0; i < 5; i++) {
        float t0 = i / 5.f, t1 = (i + 1) / 5.f;
        float y0 = y - Hh * 0.8f * t0, y1 = y - Hh * 0.8f * t1;
        float w0 = lerpf(bw, tw, t0), w1 = lerpf(bw, tw, t1);
        float q[] = {x - w0, y0, x + w0, y0, x + w1, y1, x - w1, y1};
        C3 c = (i & 1) ? c3hex(0xC0392B) : c3hex(0xF5F0E8);
        c = c3lerp(c, dark, 0.35f);
        g_poly(r->g, q, 4, c3col(c, 1));
    }
    g_rect(r->g, x - tw * 1.4f, y - Hh * 0.82f, tw * 2.8f, Hh * 0.03f, 1, c3col(dark, 1));
    g_rect(r->g, x - tw * 0.8f, y - Hh * 0.96f, tw * 1.6f, Hh * 0.14f, 1, c3col(dark, 1));
    g_glow(r->g, lx, ly, Hh * 0.35f, rgba(255, 240, 180, 200));
    g_rect(r->g, x - tw * 0.6f, y - Hh * 0.94f, tw * 1.2f, Hh * 0.1f, 1, rgba(255, 246, 210, 255));
    float dome[] = {x - tw, y - Hh * 0.96f, x + tw, y - Hh * 0.96f, x, y - Hh * 1.05f};
    g_poly(r->g, dome, 3, c3col(dark, 1));
}

static void boat(SR *r, float wx, float wy, float h) {
    float x = PXr(r, wx), y = PYr(r, wy), Hh = h * r->sc;
    C3 c = c3lerp(r->p.near, c3(0, 0, 0), 0.1f);
    float hull[] = {x - Hh * 0.5f, y - Hh * 0.12f, x + Hh * 0.55f, y - Hh * 0.12f, x + Hh * 0.38f, y, x - Hh * 0.36f, y};
    g_poly(r->g, hull, 4, c3col(c, 1));
    g_rect(r->g, x - Hh * 0.02f, y - Hh * 1.0f, Hh * 0.03f, Hh * 0.9f, 0, c3col(c, 1));
    float s1[] = {x + Hh * 0.03f, y - Hh * 0.95f, x + Hh * 0.45f, y - Hh * 0.18f, x + Hh * 0.03f, y - Hh * 0.18f};
    float s2[] = {x - Hh * 0.03f, y - Hh * 0.8f, x - Hh * 0.03f, y - Hh * 0.18f, x - Hh * 0.38f, y - Hh * 0.18f};
    g_poly(r->g, s1, 3, c3col(c3lerp(r->p.sun, c3(1, 1, 1), 0.6f), 0.95f));
    g_poly(r->g, s2, 3, c3col(c3lerp(r->p.haze, c3(1, 1, 1), 0.5f), 0.9f));
    g_rect(r->g, x - Hh * 0.6f, y + 1, Hh * 1.2f, Hh * 0.03f, 2, rgba(255, 255, 255, 50));
}

static void spire(SR *r, float wx, float wy, float h, C3 col) {
    float x = PXr(r, wx), y = PYr(r, wy), Hh = h * r->sc;
    float t[] = {x - Hh * 0.06f, y, x + Hh * 0.06f, y, x + Hh * 0.025f, y - Hh * 0.8f, x - Hh * 0.025f, y - Hh * 0.8f};
    g_poly(r->g, t, 4, c3col(col, 1));
    float n[] = {x - Hh * 0.025f, y - Hh * 0.8f, x + Hh * 0.025f, y - Hh * 0.8f, x, y - Hh};
    g_poly(r->g, n, 3, c3col(col, 1));
    g_glow(r->g, x, y - Hh, Hh * 0.06f, rgba(255, 60, 60, 200));
    for (int i = 0; i < 10; i++) g_rect(r->g, x - Hh * 0.03f, y - Hh * 0.1f - i * Hh * 0.06f, Hh * 0.06f, Hh * 0.008f, 0, rgba(255, 220, 160, 120));
}

static void planet(SR *r, float wx, float wy, float rad, C3 c0, C3 c1, int rings) {
    float x = PXr(r, wx), y = PYr(r, wy), R = rad * r->sc;
    float lx = x - R * 0.45f, ly = y - R * 0.4f;
    g_glow(r->g, x, y, R * 1.6f, c3col(c0, 0.25f));
    int n = 96;
    float pts[2 * 97];
    if (rings) {
        for (int i = 0; i <= n; i++) {
            float a = PI_F + PI_F * i / n;
            pts[2 * i] = x + cosf(a) * R * 1.9f; pts[2 * i + 1] = y + sinf(a) * R * 0.38f;
        }
        Path rp; path_init(&rp);
        path_stroke(&rp, pts, n + 1, R * 0.12f, 0);
        path_fill_color(&r->s, &rp, c3col(c3lerp(c1, c3(1, 1, 1), 0.4f), 0.55f));
        path_free(&rp);
    }
    Path p; path_init(&p);
    path_circle(&p, x, y, R);
    Paint pt = {PAINT_RADIAL, c3col(c3lerp(c0, c3(1, 1, 1), 0.35f), 1), c3col(c3scale(c1, 0.35f), 1), lx, ly, 0, 0, R * 2.1f};
    path_fill(&r->s, &p, &pt);
    path_free(&p);
    /* banding */
    for (int i = 0; i < 6; i++) {
        float yy = y - R + R * 2 * (i + 0.5f) / 6;
        float hw = sqrtf(fmaxf(0, R * R - (yy - y) * (yy - y)));
        g_rect(r->g, x - hw, yy - R * 0.03f, hw * 2, R * 0.06f, R * 0.03f, c3col(c1, 0.12f));
    }
    if (rings) {
        for (int i = 0; i <= n; i++) {
            float a = PI_F * i / n;
            pts[2 * i] = x + cosf(a) * R * 1.9f; pts[2 * i + 1] = y + sinf(a) * R * 0.38f;
        }
        Path rp; path_init(&rp);
        path_stroke(&rp, pts, n + 1, R * 0.12f, 0);
        path_fill_color(&r->s, &rp, c3col(c3lerp(c1, c3(1, 1, 1), 0.5f), 0.75f));
        path_free(&rp);
    }
}

static void ship(SR *r, float wx, float wy, float h) {
    float x = PXr(r, wx), y = PYr(r, wy), Hh = h * r->sc;
    g_glow(r->g, x - Hh * 1.1f, y, Hh * 0.9f, rgba(120, 200, 255, 160));
    float hull[] = {x + Hh * 1.2f, y, x + Hh * 0.2f, y - Hh * 0.22f, x - Hh * 1.0f, y - Hh * 0.18f, x - Hh * 1.1f, y + Hh * 0.08f, x - Hh * 0.2f, y + Hh * 0.2f};
    g_poly(r->g, hull, 5, rgba(28, 30, 48, 255));
    float wing[] = {x - Hh * 0.2f, y + Hh * 0.05f, x - Hh * 0.9f, y + Hh * 0.55f, x - Hh * 0.6f, y + Hh * 0.05f};
    g_poly(r->g, wing, 3, rgba(40, 44, 66, 255));
    g_line(r->g, x - Hh * 0.95f, y - Hh * 0.16f, x + Hh * 1.1f, y - Hh * 0.01f, 1.5f, rgba(200, 220, 255, 90));
    g_circle(r->g, x - Hh * 1.12f, y - Hh * 0.04f, Hh * 0.1f, rgba(200, 240, 255, 255));
    g_circle(r->g, x + Hh * 0.35f, y - Hh * 0.08f, Hh * 0.04f, rgba(255, 210, 120, 255));
}

/* ---------------- Scene kinds ---------------- */
static void mountains(SR *r, int layer) {
    const SceneDesc *d = r->d;
    uint32_t s = r->seed;
    C3 far = c3lerp(r->p.far, r->p.haze, 0.45f), mid = c3lerp(r->p.far, r->p.near, 0.45f), near = r->p.near;
    float snow = (d->tod == TOD_DAY || d->kind == SC_AURORA || d->tod == TOD_DAWN) ? 0.5f : 0.f;
    float hy = r->hy;
    if (layer == 1) {
        ridge(r, (RidgeP){hy - 0.02f, 0.3f, 1.3f, 0.9f, 5, s + 1}, c3lerp(far, r->p.h, 0.35f), c3lerp(far, r->p.haze, 0.5f), 1, 0.35f, snow);
        haze_band(r, hy - 0.02f, 0.12f, r->p.haze, 0.5f);
        ridge(r, (RidgeP){hy + 0.04f, 0.24f, 1.8f, 0.8f, 5, s + 2}, far, c3lerp(far, mid, 0.5f), 1, 0.3f, snow * 0.7f);
        haze_band(r, hy + 0.04f, 0.1f, r->p.haze, 0.35f);
    } else if (layer == 2) {
        ridge(r, (RidgeP){hy + 0.14f, 0.2f, 2.2f, 0.6f, 5, s + 3}, mid, c3lerp(mid, near, 0.6f), 1, 0.3f, snow * 0.4f);
        haze_band(r, hy + 0.14f, 0.08f, r->p.haze, 0.18f);
    } else if (layer == 3) {
        RidgeP np = {1.02f, 0.2f, 1.4f, 0.2f, 4, s + 4};
        ridge(r, np, c3lerp(near, mid, 0.15f), c3scale(near, 0.7f), 1, 0.4f, 0);
        if (d->element == EL_FIGURE) {
            float fx = d->sun_x + 0.16f;
            figure(r, fx, ridge_y(fx, np.base, np.amp, np.freq, np.oct, np.seed, np.sharp) + 0.004f, 0.11f, c3scale(near, 0.6f));
        } else if (d->element == EL_TOWER) {
            float fx = d->sun_x - 0.2f;
            spire(r, fx, ridge_y(fx, np.base, np.amp, np.freq, np.oct, np.seed, np.sharp) + 0.01f, 0.3f, c3scale(near, 0.8f));
        }
        /* birds */
        Rng rg = rng_make(s + 8);
        for (int i = 0; i < 7; i++) {
            float bx = PXr(r, d->sun_x - 0.3f + rng_f(&rg) * 0.4f), by = PYr(r, 0.2f + rng_f(&rg) * 0.18f), bs = r->sc * rng_range(&rg, 0.006f, 0.011f);
            float pts[] = {bx - bs, by - bs * 0.4f, bx, by, bx + bs, by - bs * 0.5f};
            Path p; path_init(&p);
            path_stroke(&p, pts, 3, fmaxf(1.f, bs * 0.22f), 0);
            path_fill_color(&r->s, &p, c3col(c3scale(near, 0.8f), 0.85f));
            path_free(&p);
        }
    }
}

static void desert(SR *r, int layer) {
    const SceneDesc *d = r->d;
    uint32_t s = r->seed;
    C3 sand = c3lerp(c3hex(0xE9B872), r->p.sun, 0.25f);
    if (d->tod == TOD_DUSK) sand = c3lerp(c3hex(0xD9865F), r->p.h, 0.3f);
    C3 shade = c3lerp(sand, r->p.near, 0.55f);
    float hy = r->hy;
    if (layer == 0 && d->element == EL_PLANET) {
        planet(r, d->sun_x - 0.55f, 0.22f, 0.13f, c3lerp(r->p.h, c3(1, 0.9f, 0.8f), 0.4f), r->p.m, 1);
        planet(r, d->sun_x - 0.2f, 0.12f, 0.035f, c3(0.9f, 0.9f, 1.f), r->p.far, 0);
    } else if (layer == 1) {
        ridge(r, (RidgeP){hy, 0.08f, 1.0f, 0.1f, 3, s + 1}, c3lerp(sand, r->p.haze, 0.6f), c3lerp(sand, r->p.haze, 0.4f), 1, 0.4f, 0);
        haze_band(r, hy, 0.08f, r->p.haze, 0.4f);
    } else if (layer == 2) {
        ridge(r, (RidgeP){hy + 0.12f, 0.14f, 1.4f, 0.05f, 3, s + 2}, c3lerp(sand, r->p.sun, 0.2f), shade, 1, 0.6f, 0);
        ridge(r, (RidgeP){hy + 0.22f, 0.12f, 1.1f, 0.05f, 3, s + 3}, c3lerp(sand, r->p.sun, 0.1f), c3lerp(shade, r->p.near, 0.3f), 1, 0.6f, 0);
    } else if (layer == 3) {
        RidgeP np = {1.0f, 0.12f, 0.9f, 0.0f, 3, s + 4};
        ridge(r, np, c3lerp(shade, r->p.near, 0.2f), c3lerp(shade, r->p.near, 0.7f), 1, 0.5f, 0);
        if (d->element == EL_FIGURE || d->element == EL_PLANET) {
            float fx = d->sun_x + 0.14f;
            figure(r, fx, ridge_y(fx, np.base, np.amp, np.freq, np.oct, np.seed, np.sharp) + 0.004f, 0.1f, c3scale(r->p.near, 0.5f));
        }
    }
}

static void canyon(SR *r, int layer) {
    const SceneDesc *d = r->d;
    Rng rg = rng_make(r->seed + layer * 31);
    C3 rock = c3lerp(c3hex(0xC65A2E), r->p.sun, 0.15f);
    float hy = r->hy;
    if (layer == 1 || layer == 2) {
        float depth = layer == 1 ? 0.6f : 0.25f;
        C3 c = c3lerp(rock, r->p.haze, depth);
        float base = layer == 1 ? hy + 0.02f : hy + 0.12f;
        float x = r->wx0 - 0.2f;
        while (x < r->wx0 + r->W / r->sc + 0.2f) {
            float w = rng_range(&rg, 0.15f, 0.4f) * (layer == 1 ? 0.8f : 1.2f);
            float h = rng_range(&rg, 0.08f, 0.22f) * (layer == 1 ? 0.8f : 1.1f);
            float px0 = PXr(r, x), px1 = PXr(r, x + w), top = PYr(r, base - h), bot = PYr(r, base + 0.02f);
            float in = (px1 - px0) * 0.12f;
            float mesa[] = {px0, bot, px0 + in * 0.6f, top + (bot - top) * 0.35f, px0 + in, top, px1 - in, top, px1 - in * 0.6f, top + (bot - top) * 0.3f, px1, bot};
            Path p; path_init(&p);
            path_poly(&p, mesa, 6);
            Paint pt = {PAINT_LINEAR, c3col(c3lerp(c, r->p.sun, 0.25f), 1), c3col(c3lerp(c, r->p.near, 0.25f), 1), px0, top, px1, bot, 0};
            path_fill(&r->s, &p, &pt);
            path_free(&p);
            g_rect(r->g, px0 + in, top, px1 - px0 - 2 * in, 2, 1, c3col(c3lerp(r->p.sun, c3(1, 1, 1), 0.3f), 0.5f));
            for (int k = 1; k < 4; k++) g_rect(r->g, px0 + in * 0.5f, top + (bot - top) * k * 0.22f, px1 - px0 - in, 1.5f, 0, c3col(c3scale(c, 0.7f), 0.35f));
            x += w * rng_range(&rg, 0.9f, 1.6f);
        }
        ridge(r, (RidgeP){base + 0.06f, 0.03f, 2.f, 0, 3, r->seed + layer}, c3lerp(c, r->p.near, 0.3f), c3lerp(c, r->p.near, 0.5f), 1, 0, 0);
        haze_band(r, base, 0.06f, r->p.haze, layer == 1 ? 0.4f : 0.15f);
    } else if (layer == 3) {
        RidgeP np = {1.0f, 0.16f, 1.2f, 0.3f, 4, r->seed + 9};
        ridge(r, np, c3lerp(rock, r->p.near, 0.6f), c3scale(r->p.near, 0.6f), 1, 0.5f, 0);
        if (d->element == EL_FIGURE) {
            float fx = d->sun_x + 0.15f;
            figure(r, fx, ridge_y(fx, np.base, np.amp, np.freq, np.oct, np.seed, np.sharp) + 0.004f, 0.11f, c3scale(r->p.near, 0.4f));
        }
    }
}

static void ocean(SR *r, int layer) {
    const SceneDesc *d = r->d;
    float hy = r->hy;
    float y0 = PYr(r, hy);
    if (layer == 1) {
        RidgeP hp = {hy + 0.002f, 0.06f, 2.f, 0.3f, 4, r->seed + 2};
        /* distant headland only on one side */
        Path p; path_init(&p);
        int n = r->W / 6 + 2;
        float *pts = xmalloc(sizeof(float) * 2 * (n + 2));
        for (int i = 0; i < n; i++) {
            float px = (float)(i * 6), wx = WXr(r, px);
            float side = smoothstep(d->sun_x + 0.35f, d->sun_x + 0.6f, wx);
            pts[2 * i] = px; pts[2 * i + 1] = PYr(r, lerpf(hy + 0.002f, ridge_y(wx, hp.base, hp.amp, hp.freq, hp.oct, hp.seed, hp.sharp), side));
        }
        pts[2 * n] = (float)r->W; pts[2 * n + 1] = y0 + 2; pts[2 * n + 2] = 0; pts[2 * n + 3] = y0 + 2;
        path_poly(&p, pts, n + 2);
        path_fill_color(&r->s, &p, c3col(c3lerp(r->p.far, r->p.haze, 0.4f), 1));
        path_free(&p);
        free(pts);
    } else if (layer == 2) {
        C3 top = c3lerp(r->p.h, r->p.water, 0.35f), bot = c3scale(r->p.water, 0.55f);
        if (d->tod == TOD_DAY) { top = c3hex(0x5FB4E6); bot = c3hex(0x0B4F8A); }
        g_rect_vgrad(r->g, 0, y0, (float)r->W, (float)r->H - y0, 0, c3col(top, 1), c3col(bot, 1));
        /* sky reflection near horizon */
        g_rect_vgrad(r->g, 0, y0, (float)r->W, (r->H - y0) * 0.35f, 0, c3col(r->p.h, 0.5f), c3col(r->p.h, 0));
        Rng rg = rng_make(r->seed + 5);
        float sx = PXr(r, d->sun_x);
        /* glitter path */
        int n = (int)(1400 * r->W / 1920.f) + 200;
        for (int i = 0; i < n; i++) {
            float t = powf(rng_f(&rg), 1.6f);
            float y = y0 + 2 + t * (r->H - y0);
            float spread = (0.03f + t * 0.35f) * r->sc;
            float x = sx + (rng_f(&rg) + rng_f(&rg) - 1.f) * spread;
            float len = (2 + t * 26) * r->sc / 1080.f * rng_range(&rg, 0.5f, 1.5f);
            float a = (1.f - fabsf(x - sx) / (spread + 1)) * (0.25f + 0.55f * (1 - t));
            g_rect(r->g, x - len, y, len * 2, fmaxf(1.f, (1 + t * 2.5f) * r->sc / 1080.f), 1, c3col(c3lerp(r->p.sun, c3(1, 1, 1), 0.4f), a));
        }
        /* wave lines */
        for (int i = 0; i < 260 * r->W / 1920 + 30; i++) {
            float t = powf(rng_f(&rg), 1.4f);
            float y = y0 + 3 + t * (r->H - y0);
            float x = rng_f(&rg) * r->W, len = (10 + t * 140) * r->sc / 1080.f;
            g_rect(r->g, x, y, len, fmaxf(1.f, t * 3 * r->sc / 1080.f), 1, c3col(c3lerp(r->p.h, c3(1, 1, 1), 0.3f), 0.08f + 0.1f * t));
        }
        if (d->element == EL_BOAT) boat(r, d->sun_x - 0.32f, hy + 0.08f, 0.09f);
    } else if (layer == 3) {
        if (d->element == EL_LIGHTHOUSE) {
            RidgeP cp = {1.05f, 0.42f, 1.6f, 0.2f, 4, r->seed + 7};
            float cx0 = d->sun_x + 0.32f;
            /* cliff only on the right part */
            Path p; path_init(&p);
            int n = r->W / 6 + 2;
            float *pts = xmalloc(sizeof(float) * 2 * (n + 2));
            for (int i = 0; i < n; i++) {
                float px = (float)(i * 6), wx = WXr(r, px);
                float m = smoothstep(cx0 - 0.08f, cx0 + 0.06f, wx);
                pts[2 * i] = px; pts[2 * i + 1] = PYr(r, lerpf(1.08f, ridge_y(wx, cp.base, cp.amp, cp.freq, cp.oct, cp.seed, cp.sharp), m));
            }
            pts[2 * n] = (float)r->W + 4; pts[2 * n + 1] = (float)r->H + 4; pts[2 * n + 2] = -4; pts[2 * n + 3] = (float)r->H + 4;
            path_poly(&p, pts, n + 2);
            path_fill_color(&r->s, &p, c3col(c3scale(r->p.near, 0.85f), 1));
            path_free(&p);
            free(pts);
            float lx = cx0 + 0.12f;
            lighthouse(r, lx, ridge_y(lx, cp.base, cp.amp, cp.freq, cp.oct, cp.seed, cp.sharp) + 0.01f, 0.26f);
        } else if (d->tod == TOD_DAY || d->tod == TOD_GOLDEN) {
            /* beach & foam */
            RidgeP bp = {1.0f, 0.08f, 0.8f, 0, 3, r->seed + 9};
            ridge(r, bp, c3lerp(c3hex(0xF3D9A4), r->p.sun, 0.2f), c3hex(0xC9A26B), 1, 0, 0);
            Path f; path_init(&f);
            int n = r->W / 6 + 1;
            float *pts = xmalloc(sizeof(float) * 2 * n);
            for (int i = 0; i < n; i++) {
                float px = (float)(i * 6);
                pts[2 * i] = px; pts[2 * i + 1] = PYr(r, ridge_y(WXr(r, px), bp.base, bp.amp, bp.freq, bp.oct, bp.seed, bp.sharp)) - 2;
            }
            path_stroke(&f, pts, n, 4.f * r->sc / 1080.f + 1, 0);
            path_fill_color(&r->s, &f, rgba(255, 255, 255, 200));
            path_free(&f);
            free(pts);
        } else {
            RidgeP rp = {1.03f, 0.1f, 2.4f, 0.5f, 4, r->seed + 11};
            ridge(r, rp, c3scale(r->p.near, 0.9f), c3scale(r->p.near, 0.6f), 1, 0.4f, 0);
        }
    }
}

static void city(SR *r, int layer) {
    const SceneDesc *d = r->d;
    float hy = r->hy;
    int night = d->tod == TOD_NIGHT || d->tod == TOD_DUSK;
    Rng rg = rng_make(r->seed + layer * 101);
    if (layer == 1 || layer == 2) {
        float depth = layer == 1 ? 0.55f : 0.0f;
        C3 bc = c3lerp(c3lerp(r->p.near, r->p.far, 0.4f), r->p.haze, depth);
        float base = PYr(r, hy + (layer == 1 ? -0.005f : 0.0f));
        float x = -10;
        while (x < r->W + 10) {
            float w = rng_range(&rg, 0.02f, 0.06f) * r->sc * (layer == 1 ? 0.8f : 1.f);
            float h = (powf(rng_f(&rg), 2.f) * 0.32f + 0.05f) * r->sc * (layer == 1 ? 0.9f : 0.85f);
            float bx = x, by = base - h;
            C3 c = c3lerp(bc, r->p.near, rng_f(&rg) * 0.3f);
            g_rect_vgrad(r->g, bx, by, w, h + 2, 0, c3col(c3lerp(c, r->p.sun, layer == 1 ? 0.15f : 0.08f), 1), c3col(c, 1));
            /* edge light */
            g_rect(r->g, bx + w - 2, by, 2, h, 0, c3col(r->p.sun, 0.12f));
            if (rng_f(&rg) < 0.3f) g_rect(r->g, bx + w * 0.4f, by - h * 0.08f, fmaxf(1.f, w * 0.06f), h * 0.08f, 0, c3col(c, 1));
            if (layer == 2 || night) {
                float ww = fmaxf(1.5f, w * 0.1f), wh = fmaxf(1.5f, r->sc * 0.006f);
                for (float wy = by + wh * 2; wy < base - wh * 2; wy += wh * 2.6f)
                    for (float wx = bx + ww; wx < bx + w - ww * 1.5f; wx += ww * 2.f) {
                        if (rng_f(&rg) < (night ? 0.45f : 0.12f)) {
                            float warm = rng_f(&rg);
                            uint32_t lc = warm < 0.7f ? rgba(255, 214, 140, night ? 230 : 120) : rgba(170, 220, 255, night ? 210 : 100);
                            if (layer == 1) lc = col_alpha(lc, 0.5f);
                            g_rect(r->g, wx, wy, ww, wh, 0, lc);
                        }
                    }
            }
            x += w + rng_range(&rg, 0.f, 0.006f) * r->sc;
        }
        if (layer == 1) haze_band(r, hy - 0.05f, 0.12f, r->p.haze, 0.35f);
        if (layer == 2 && d->element == EL_TOWER) spire(r, d->sun_x - 0.25f, hy, 0.55f, c3scale(r->p.near, 0.9f));
    } else if (layer == 3) {
        float y0 = PYr(r, hy);
        g_rect_vgrad(r->g, 0, y0, (float)r->W, r->H - y0, 0, c3col(c3lerp(r->p.near, r->p.h, 0.3f), 1), c3col(c3scale(r->p.near, 0.5f), 1));
        /* light reflections on water */
        for (int i = 0; i < 500 * r->W / 1920 + 40; i++) {
            float x = rng_f(&rg) * r->W, y = y0 + 2 + powf(rng_f(&rg), 1.3f) * (r->H - y0);
            float len = rng_range(&rg, 4, 40) * r->sc / 1080.f;
            uint32_t lc = rng_f(&rg) < 0.7f ? rgba(255, 210, 140, 70) : rgba(160, 210, 255, 60);
            if (!night) lc = col_alpha(lc, 0.5f);
            g_rect(r->g, x, y, len, fmaxf(1.f, r->sc / 600.f), 1, lc);
        }
        float sx = PXr(r, d->sun_x);
        for (int i = 0; i < 160; i++) {
            float t = rng_f(&rg);
            float y = y0 + 2 + t * (r->H - y0), x = sx + (rng_f(&rg) - 0.5f) * (0.05f + t * 0.2f) * r->sc;
            g_rect(r->g, x - 10, y, 20 + t * 30, 2, 1, c3col(r->p.sun, 0.25f * (1 - t)));
        }
        /* pier */
        g_rect(r->g, 0, PYr(r, 0.93f), (float)r->W * 0.45f, PYr(r, 0.012f), 0, c3col(c3scale(r->p.near, 0.4f), 1));
        for (int i = 0; i < 10; i++) g_rect(r->g, i * r->W * 0.045f + 6, PYr(r, 0.93f), PYr(r, 0.006f), PYr(r, 0.08f), 0, c3col(c3scale(r->p.near, 0.4f), 1));
    }
}

static void space(SR *r, int layer) {
    const SceneDesc *d = r->d;
    if (layer == 1) {
        if (d->element == EL_PLANET) planet(r, d->sun_x, d->sun_y + 0.05f, 0.24f, c3lerp(r->p.h, c3(1, 0.85f, 0.7f), 0.5f), r->p.m, 1);
        else planet(r, d->sun_x - 0.45f, 0.3f, 0.16f, c3lerp(r->p.h, c3(0.5f, 0.8f, 1.f), 0.5f), r->p.z, 0);
        planet(r, d->sun_x + 0.5f, 0.18f, 0.03f, c3(0.85f, 0.85f, 0.95f), c3(0.3f, 0.3f, 0.4f), 0);
    } else if (layer == 2) {
        RidgeP rp = {0.98f, 0.14f, 1.5f, 0.6f, 5, r->seed + 3};
        ridge(r, rp, c3lerp(r->p.near, r->p.h, 0.2f), c3scale(r->p.near, 0.5f), 1, 0.6f, 0);
    } else if (layer == 3) {
        if (d->element == EL_SHIP) ship(r, d->sun_x + 0.2f, 0.42f, 0.06f);
        Rng rg = rng_make(r->seed + 13);
        for (int i = 0; i < 6; i++) {
            float x = rng_f(&rg) * r->W, y = rng_f(&rg) * r->H * 0.7f, rr = rng_range(&rg, 3, 12) * r->sc / 1080.f;
            g_circle(r->g, x, y, rr, rgba(60, 50, 70, 255));
            g_circle(r->g, x - rr * 0.3f, y - rr * 0.3f, rr * 0.5f, rgba(140, 120, 150, 120));
        }
    }
}

static void forest(SR *r, int layer) {
    const SceneDesc *d = r->d;
    float hy = r->hy;
    C3 fog = c3lerp(r->p.haze, c3(1, 1, 1), 0.2f);
    if (layer == 1) {
        ridge(r, (RidgeP){hy - 0.05f, 0.18f, 1.2f, 0.3f, 4, r->seed + 1}, c3lerp(r->p.far, fog, 0.5f), c3lerp(r->p.far, fog, 0.6f), 1, 0.2f, 0);
        tree_line(r, hy + 0.02f, 0.05f, 0.05f, 0.09f, 1.6f, c3lerp(r->p.far, fog, 0.45f), 1, r->seed + 2);
        haze_band(r, hy + 0.02f, 0.1f, fog, 0.6f);
    } else if (layer == 2) {
        tree_line(r, hy + 0.14f, 0.06f, 0.1f, 0.18f, 1.3f, c3lerp(r->p.far, r->p.near, 0.5f), 1, r->seed + 3);
        haze_band(r, hy + 0.15f, 0.1f, fog, 0.35f);
        /* light rays */
        float sx = PXr(r, d->sun_x), sy = PYr(r, d->sun_y);
        Rng rg = rng_make(r->seed + 5);
        for (int i = 0; i < 7; i++) {
            float a = PI_F * 0.5f + rng_range(&rg, -0.7f, 0.7f), w = rng_range(&rg, 0.02f, 0.06f);
            float L = r->sc * 1.4f;
            float ray[] = {sx, sy, sx + cosf(a - w) * L, sy + sinf(a - w) * L, sx + cosf(a + w) * L, sy + sinf(a + w) * L};
            Path p; path_init(&p);
            path_poly(&p, ray, 3);
            Paint pt = {PAINT_LINEAR, c3col(r->p.sun, 0.16f), c3col(r->p.sun, 0), sx, sy, sx, sy + L * 0.8f, 0};
            path_fill(&r->s, &p, &pt);
            path_free(&p);
        }
    } else if (layer == 3) {
        RidgeP gp = {1.0f, 0.06f, 1.f, 0, 3, r->seed + 7};
        ridge(r, gp, c3scale(r->p.near, 0.9f), c3scale(r->p.near, 0.6f), 1, 0.3f, 0);
        Path p; path_init(&p);
        Rng rg = rng_make(r->seed + 6);
        for (int side = 0; side < 2; side++)
            for (int i = 0; i < 3; i++) {
                float wx = side ? r->wx0 + r->W / r->sc - 0.05f - i * 0.09f : r->wx0 + 0.04f + i * 0.08f;
                { float th = r->sc * rng_range(&rg, 0.5f, 0.85f); pine(&p, PXr(r, wx + rng_range(&rg, -0.02f, 0.02f)), (float)r->H + th * 0.22f, th); }
            }
        path_fill_color(&r->s, &p, c3col(c3scale(r->p.near, 0.7f), 1));
        path_free(&p);
        if (d->element == EL_FIGURE) {
            float fx = d->sun_x + 0.05f;
            figure(r, fx, ridge_y(fx, gp.base, gp.amp, gp.freq, gp.oct, gp.seed, gp.sharp) + 0.004f, 0.09f, c3scale(r->p.near, 0.5f));
        }
        /* fireflies / motes */
        for (int i = 0; i < 40; i++) {
            float x = rng_f(&rg) * r->W, y = PYr(r, hy + rng_f(&rg) * 0.3f);
            g_glow(r->g, x, y, 6 * r->sc / 1080.f, c3col(r->p.sun, 0.5f));
        }
    }
}

static void synth(SR *r, int layer) {
    float hy = r->hy;
    float y0 = PYr(r, hy);
    if (layer == 1) {
        RidgeP rp = {hy + 0.005f, 0.2f, 2.f, 0.9f, 3, r->seed + 2};
        ridge(r, rp, c3hex(0x3A1466), c3hex(0x12052E), 1, 0.0f, 0);
        int n = r->W / 6 + 1;
        float *pts = xmalloc(sizeof(float) * 2 * n);
        for (int i = 0; i < n; i++) {
            float px = (float)(i * 6);
            pts[2 * i] = px; pts[2 * i + 1] = PYr(r, ridge_y(WXr(r, px), rp.base, rp.amp, rp.freq, rp.oct, rp.seed, rp.sharp));
        }
        Path p; path_init(&p);
        path_stroke(&p, pts, n, 2.f * r->sc / 1080.f + 0.5f, 0);
        path_fill_color(&r->s, &p, rgba(255, 90, 200, 230));
        path_free(&p);
        free(pts);
    } else if (layer == 2) {
        g_rect_vgrad(r->g, 0, y0, (float)r->W, r->H - y0, 0, c3col(c3hex(0x2A0B4A), 1), c3col(c3hex(0x07010F), 1));
        g_rect_vgrad(r->g, 0, y0, (float)r->W, PYr(r, 0.05f), 0, rgba(255, 80, 180, 140), rgba(255, 80, 180, 0));
        float vx = PXr(r, r->d->sun_x);
        float lw = fmaxf(1.f, 2.f * r->sc / 1080.f);
        for (int i = -30; i <= 30; i++) {
            float bx = vx + i * r->sc * 0.12f;
            g_line(r->g, vx + i * r->sc * 0.004f, y0, bx + (bx - vx) * 3.f, (float)r->H + 40, lw, rgba(255, 70, 200, 180));
        }
        for (int i = 1; i < 18; i++) {
            float t = i / 18.f;
            float y = y0 + (r->H - y0) * t * t;
            g_rect(r->g, 0, y, (float)r->W, lw * (0.6f + t), 0, rgba(255, 70, 200, (int)(80 + 150 * t)));
        }
    } else if (layer == 3) {
        Path p; path_init(&p);
        for (int side = 0; side < 2; side++) {
            float bx = side ? r->W * 0.92f : r->W * 0.07f, by = (float)r->H + 5, hh = r->sc * 0.45f;
            float tr[] = {bx - hh * 0.02f, by, bx + hh * 0.02f, by, bx + hh * 0.03f * (side ? -1 : 1) + hh * 0.012f, by - hh, bx + hh * 0.03f * (side ? -1 : 1) - hh * 0.012f, by - hh};
            path_poly(&p, tr, 4);
            for (int k = 0; k < 7; k++) {
                float a = -PI_F * 0.5f + (k - 3) * 0.45f;
                float cx = bx + hh * 0.03f * (side ? -1 : 1), cy = by - hh;
                float ex = cx + cosf(a) * hh * 0.35f, ey = cy + sinf(a) * hh * 0.18f + hh * 0.08f * fabsf((float)(k - 3));
                float leaf[] = {cx, cy, (cx + ex) * 0.5f, (cy + ey) * 0.5f - hh * 0.05f, ex, ey, (cx + ex) * 0.5f, (cy + ey) * 0.5f + hh * 0.01f};
                path_poly(&p, leaf, 4);
            }
        }
        path_fill_color(&r->s, &p, rgba(10, 2, 20, 255));
        path_free(&p);
    }
}

static void aurora_land(SR *r, int layer) {
    float hy = r->hy;
    C3 snow = c3lerp(c3(0.85f, 0.9f, 1.f), r->p.h, 0.4f);
    if (layer == 1) {
        ridge(r, (RidgeP){hy, 0.26f, 1.5f, 0.85f, 5, r->seed + 1}, c3lerp(snow, r->p.far, 0.55f), c3lerp(r->p.far, r->p.near, 0.4f), 1, 0.25f, 0.6f);
        haze_band(r, hy, 0.06f, r->p.h, 0.25f);
    } else if (layer == 2) {
        float y0 = PYr(r, hy + 0.06f);
        ridge(r, (RidgeP){hy + 0.08f, 0.04f, 1.f, 0, 3, r->seed + 2}, c3lerp(snow, r->p.m, 0.5f), c3lerp(snow, r->p.near, 0.6f), 1, 0.4f, 0);
        (void)y0;
    } else if (layer == 3) {
        tree_line(r, 1.0f, 0.06f, 0.12f, 0.3f, 0.7f, c3scale(r->p.near, 0.8f), 1, r->seed + 3);
        if (r->d->element == EL_FIGURE) figure(r, r->d->sun_x + 0.1f, 0.955f, 0.1f, c3scale(r->p.near, 0.6f));
    }
}

static void paper(SR *r, int layer) {
    const SceneDesc *d = r->d;
    C3 cols[5] = {c3hex(0xF6C1A6), c3hex(0xF49FB6), c3hex(0xA98BE0), c3hex(0x6E7FD8), c3hex(0x3E4FA8)};
    if (d->tod == TOD_DUSK) { cols[0] = c3hex(0xFFB38A); cols[1] = c3hex(0xF57C9C); cols[2] = c3hex(0xB45FA8); cols[3] = c3hex(0x6B3F9A); cols[4] = c3hex(0x2D1F5E); }
    int from = layer == 1 ? 0 : (layer == 2 ? 2 : 4), to = layer == 1 ? 2 : (layer == 2 ? 4 : 5);
    if (layer == 0) {
        /* paper clouds */
        Rng rg = rng_make(r->seed + 4);
        for (int i = 0; i < 4; i++) {
            float x = PXr(r, d->sun_x - 0.7f + i * 0.45f + rng_range(&rg, -0.1f, 0.1f)), y = PYr(r, 0.12f + rng_f(&rg) * 0.2f), w = r->sc * rng_range(&rg, 0.16f, 0.26f);
            g_shadow(r->g, x, y + 6, w, w * 0.22f, w * 0.11f, 12, rgba(90, 50, 80, 50));
            g_rect(r->g, x, y, w, w * 0.22f, w * 0.11f, rgba(255, 255, 255, 235));
            g_circle(r->g, x + w * 0.35f, y, w * 0.15f, rgba(255, 255, 255, 235));
            g_circle(r->g, x + w * 0.6f, y + w * 0.02f, w * 0.11f, rgba(255, 255, 255, 235));
        }
        return;
    }
    for (int i = from; i < to; i++) {
        RidgeP rp = {r->hy - 0.1f + i * 0.1f, 0.2f - i * 0.015f, 1.1f + i * 0.35f, 0.0f, 3, r->seed + i * 13};
        int n = r->W / 6 + 3;
        float *pts = xmalloc(sizeof(float) * 2 * (n + 2));
        for (int k = 0; k < n; k++) {
            float px = (float)(k * 6 - 6);
            pts[2 * k] = px; pts[2 * k + 1] = PYr(r, ridge_y(WXr(r, px), rp.base, rp.amp, rp.freq, rp.oct, rp.seed, rp.sharp));
        }
        pts[2 * n] = (float)r->W + 6; pts[2 * n + 1] = (float)r->H + 6; pts[2 * n + 2] = -6; pts[2 * n + 3] = (float)r->H + 6;
        /* soft paper shadow cast upward */
        for (int s = 3; s >= 1; s--) {
            Path sp; path_init(&sp);
            path_transform(&sp, 1, 0, 0, 0, 1, -s * 3.f * r->sc / 1080.f * 2);
            path_poly(&sp, pts, n + 2);
            path_fill_color(&r->s, &sp, rgba(60, 20, 60, 22));
            path_free(&sp);
        }
        Path p; path_init(&p);
        path_poly(&p, pts, n + 2);
        Paint pt = {PAINT_LINEAR, c3col(c3lerp(cols[i], c3(1, 1, 1), 0.12f), 1), c3col(cols[i], 1), 0, PYr(r, rp.base - rp.amp), 0, PYr(r, rp.base + 0.1f), 0};
        path_fill(&r->s, &p, &pt);
        path_free(&p);
        if (i == 2 && d->element == EL_TREE) {
            for (int k = 0; k < 3; k++) {
                float fx = d->sun_x + 0.18f + k * 0.11f;
                lone_tree(r, fx, ridge_y(fx, rp.base, rp.amp, rp.freq, rp.oct, rp.seed, rp.sharp) + 0.01f, 0.14f - k * 0.02f, cols[4], 1);
            }
        }
        if (i == 3 && d->element == EL_TOWER) {
            float fx = d->sun_x - 0.22f, fy = ridge_y(fx, rp.base, rp.amp, rp.freq, rp.oct, rp.seed, rp.sharp) + 0.02f;
            float x = PXr(r, fx), y = PYr(r, fy), H = r->sc * 0.28f;
            g_shadow(r->g, x - H * 0.1f, y - H, H * 0.2f, H, 4, 14, rgba(40, 20, 50, 60));
            g_rect(r->g, x - H * 0.1f, y - H, H * 0.2f, H, 3, rgba(255, 244, 230, 255));
            float roof[] = {x - H * 0.15f, y - H, x + H * 0.15f, y - H, x, y - H * 1.35f};
            g_poly(r->g, roof, 3, c3col(cols[1], 1));
            g_rect(r->g, x - H * 0.04f, y - H * 0.8f, H * 0.08f, H * 0.12f, H * 0.04f, rgba(255, 200, 80, 255));
            g_glow(r->g, x, y - H * 0.74f, H * 0.2f, rgba(255, 200, 80, 120));
        }
        free(pts);
    }
}

static void lake(SR *r, int layer) {
    /* Mountains above the waterline; the reflection is added in post. */
    if (layer == 1) {
        ridge(r, (RidgeP){r->hy - 0.0f, 0.36f, 1.2f, 0.85f, 5, r->seed + 1}, c3lerp(r->p.far, r->p.h, 0.3f), c3lerp(r->p.far, r->p.haze, 0.4f), 1, 0.35f, r->d->tod == TOD_NIGHT ? 0.3f : 0.6f);
        haze_band(r, r->hy - 0.03f, 0.1f, r->p.haze, 0.4f);
    } else if (layer == 2) {
        tree_line(r, r->hy + 0.003f, 0.01f, 0.03f, 0.07f, 1.8f, c3lerp(r->p.far, r->p.near, 0.6f), 1, r->seed + 2);
    } else if (layer == 3 && r->d->element == EL_TREE) {
        lone_tree(r, r->d->sun_x + 0.25f, r->hy + 0.004f, 0.16f, c3scale(r->p.near, 0.8f), 0);
    }
}

static void draw_layer(SR *r, int layer) {
    if (layer == 0) { sky_layer(r); }
    switch (r->d->kind) {
    case SC_MOUNTAINS: if (layer) mountains(r, layer); break;
    case SC_DESERT: desert(r, layer); break;
    case SC_OCEAN: if (layer) ocean(r, layer); break;
    case SC_CITY: if (layer) city(r, layer); break;
    case SC_SPACE: if (layer) space(r, layer); break;
    case SC_FOREST: if (layer) forest(r, layer); break;
    case SC_SYNTH: if (layer) synth(r, layer); break;
    case SC_AURORA: if (layer) aurora_land(r, layer); break;
    case SC_CANYON: if (layer) canyon(r, layer); break;
    case SC_PAPER: paper(r, layer); break;
    case SC_LAKE: if (layer) lake(r, layer); break;
    }
}

static void post_reflect(SR *r) {
    int hyp = (int)PYr(r, r->hy);
    C3 wc = c3lerp(r->p.water, r->p.h, 0.3f);
    uint32_t wcol = c3col(wc, 1);
    for (int y = hyp; y < r->H; y++) {
        int src = 2 * hyp - y - 1;
        if (src < 0) src = 0;
        float t = (float)(y - hyp) / (r->H - hyp);
        uint32_t *row = r->s.px + (size_t)y * r->s.stride;
        const uint32_t *srow = r->s.px + (size_t)src * r->s.stride;
        int ripple = (int)(sinf(y * 0.9f) * (1 + t * 4) * r->sc / 1080.f);
        uint32_t mixa = (uint32_t)(255 * (0.18f + 0.35f * t));
        for (int x = 0; x < r->W; x++) {
            int sx = clampi(x + ripple, 0, r->W - 1);
            uint32_t c = lerp256(srow[sx], wcol, mixa);
            row[x] = c | 0xFF000000u;
        }
        if ((y - hyp) % 7 == 3 && t > 0.05f) {
            for (int x = 0; x < r->W; x++) row[x] = lerp256(row[x], 0xFFFFFFFFu, (uint32_t)(10 * (1 - t)));
        }
    }
    /* shoreline highlight */
    g_rect(r->g, 0, (float)hyp - 1, (float)r->W, 2, 0, c3col(r->p.h, 0.5f));
}

static void post_finish(SR *r, float vignette, float grain) {
    float cx = r->W * 0.5f, cy = r->H * 0.5f, inv = 1.f / (cx * cx + cy * cy);
    uint32_t seed = r->seed * 31 + 7;
    for (int y = 0; y < r->H; y++) {
        uint32_t *row = r->s.px + (size_t)y * r->s.stride;
        float dy = (y - cy) * (y - cy);
        for (int x = 0; x < r->W; x++) {
            float d2 = ((x - cx) * (x - cx) + dy) * inv;
            float v = 1.f - vignette * d2 * d2 * 1.3f;
            int n = grain > 0 ? (int)((hash2i(x, y, seed) & 255) - 128) * (int)(grain * 255) / 2048 : 0;
            uint32_t c = row[x];
            int rr = (int)(((c >> 16) & 255) * v) + n, gg = (int)(((c >> 8) & 255) * v) + n, bb = (int)((c & 255) * v) + n;
            row[x] = 0xFF000000u | ((uint32_t)clampi(rr, 0, 255) << 16) | ((uint32_t)clampi(gg, 0, 255) << 8) | (uint32_t)clampi(bb, 0, 255);
        }
    }
}

static void sr_init(SR *r, Texture *dst, const SceneDesc *d, float wx0, float ww) {
    r->s = tex_surface(dst);
    r->g = gfx_new_immediate(r->s);
    r->W = dst->w; r->H = dst->h;
    r->sc = dst->w / ww;
    r->wx0 = wx0;
    r->d = d;
    r->p = make_pal(d);
    r->hy = horizon_of(d);
    r->seed = d->seed * 2654435761u + 12345u;
}

void scene_render(Texture *dst, const SceneDesc *d, float wx0, float ww, int mask) {
    SR r;
    sr_init(&r, dst, d, wx0, ww);
    for (int l = 0; l < 4; l++)
        if (mask & (1 << l)) draw_layer(&r, l);
    if (d->reflect || d->kind == SC_LAKE) post_reflect(&r);
    post_finish(&r, 0.32f, d->photo ? 0.06f : 0.1f);
    gfx_free(r.g);
    dst->opaque = 1;
}

void scene_render_layers(Texture *layers[4], const SceneDesc *d, float ww) {
    for (int l = 0; l < 4; l++) {
        SR r;
        sr_init(&r, layers[l], d, d->sun_x - ww * 0.5f + 0.0f, ww);
        if (l == 0) {
            draw_layer(&r, 0);
        } else {
            tex_clear(layers[l], 0);
            draw_layer(&r, l);
        }
        gfx_free(r.g);
        layers[l]->opaque = l == 0;
        /* first row with any visible pixel: lets the compositor skip empty sky */
        Texture *t = layers[l];
        t->ytop = 0;
        if (l) {
            int y = 0;
            for (; y < t->h; y++) {
                const uint32_t *row = t->px + (size_t)y * t->w;
                int any = 0;
                for (int x = 0; x < t->w && !any; x++) any = row[x] >> 24;
                if (any) break;
            }
            t->ytop = MAXI(0, y - 2);
        }
        layers[l]->ready = 1;
    }
}

/* ======================= Posters ======================= */
static void upper(char *dst, const char *s, int n) {
    int i = 0;
    for (; s[i] && i < n - 1; i++) dst[i] = (s[i] >= 'a' && s[i] <= 'z') ? s[i] - 32 : s[i];
    dst[i] = 0;
}

static Texture *render_poster(Movie *m) {
    const int PW = 360, PH = 540;
    Texture *t = tex_new(PW, PH);
    float ww = 0.667f;
    float focus = m->scene.element == EL_LIGHTHOUSE ? m->scene.sun_x + 0.3f : m->scene.sun_x;
    float wx0 = clampf(focus - ww * 0.42f, 0.f, 1.78f - ww);
    scene_render(t, &m->scene, wx0, ww, 15);
    Surface s = tex_surface(t);
    Gfx *g = gfx_new_immediate(s);
    char title[128];
    upper(title, m->title, sizeof title);
    int style = m->title_style;
    /* Scrim for legibility. */
    if (style == 0) g_rect_vgrad(g, 0, 0, PW, PH * 0.45f, 0, rgba(0, 0, 0, 120), rgba(0, 0, 0, 0));
    g_rect_vgrad(g, 0, PH * 0.55f, PW, PH * 0.45f, 0, rgba(0, 0, 0, 0), rgba(0, 0, 0, 170));
    int font = (style == 2) ? FONT_THIN : FONT_BLACK;
    float track = style == 2 ? 6.f : 2.f;
    /* split into two lines if needed */
    char l1[128], l2[128] = "";
    snprintf(l1, sizeof l1, "%s", title);
    float maxw = PW * 0.86f;
    /* width(size) = size/100 * w100 + tracking * (glyphs - 1) */
#define FIT(str) ((maxw - track * (float)(strlen(str) > 1 ? strlen(str) - 1 : 0)) * 100.f / fmaxf(1.f, text_width(font, 100, str, 0)))
    float size = FIT(l1);
    if (size < 46 && strchr(title, ' ')) {
        int len = (int)strlen(title), best = -1;
        for (int i = 0; i < len; i++)
            if (title[i] == ' ' && (best < 0 || abs(i - len / 2) < abs(best - len / 2))) best = i;
        memcpy(l1, title, best); l1[best] = 0;
        snprintf(l2, sizeof l2, "%s", title + best + 1);
        size = fminf(FIT(l1), FIT(l2));
    }
#undef FIT
    size = fminf(size, 74.f);
    float lh = size * 0.98f;
    int lines = l2[0] ? 2 : 1;
    float block = font_cap(font, size) + (lines - 1) * lh;
    float ty = style == 0 ? PH * 0.1f : (style == 1 ? PH * 0.74f - block : PH * 0.44f - block * 0.5f);
    uint32_t tc = WHITE;
    g_text_shadow(g, font, size, PW * 0.5f, ty, l1, tc, ALIGN_CENTER, track);
    if (lines == 2) g_text_shadow(g, font, size, PW * 0.5f, ty + lh, l2, tc, ALIGN_CENTER, track);
    /* tagline */
    char tag[128];
    upper(tag, m->tagline, sizeof tag);
    float tgy = style == 0 ? ty + block + 18 : ty - 26;
    g_text_shadow(g, FONT_MEDIUM, 11, PW * 0.5f, tgy, tag, rgba(255, 255, 255, 230), ALIGN_CENTER, 2.2f);
    /* billing block */
    char bill[256];
    snprintf(bill, sizeof bill, "A KEEL STUDIOS PRODUCTION  ·  A FILM BY %s", m->director);
    upper(bill, bill, sizeof bill);
    g_text_fit(g, FONT_REGULAR, 7.5f, PW * 0.5f, PH - 50, bill, rgba(255, 255, 255, 170), PW * 0.9f, ALIGN_CENTER);
    char cast[256];
    snprintf(cast, sizeof cast, "%s   %s   %s", m->cast[0], m->cast[1], m->cast[2]);
    upper(cast, cast, sizeof cast);
    g_text_fit(g, FONT_SEMIBOLD, 8.5f, PW * 0.5f, PH - 64, cast, rgba(255, 255, 255, 210), PW * 0.9f, ALIGN_CENTER);
    g_text(g, FONT_BOLD, 9, PW * 0.5f, PH - 32, m->is_new ? "NOW STREAMING" : "ONLY ON KEEL", rgba(255, 255, 255, 220), ALIGN_CENTER, 3);
    gfx_free(g);
    t->opaque = 1;
    tex_build_mips(t, 40);
    return t;
}

/* ======================= Album covers ======================= */
static Texture *render_cover(Album *a, int idx) {
    const int S = 600;
    Texture *t = tex_new(S, S);
    Surface s = tex_surface(t);
    Gfx *g = gfx_new_immediate(s);
    Rng rg = rng_make(idx * 97 + 5);
    C3 c0 = c3hex(a->c0 & 0xFFFFFF), c1 = c3hex(a->c1 & 0xFFFFFF), c2 = c3hex(a->c2 & 0xFFFFFF);
    switch (a->style) {
    case 0: { /* synthwave sun */
        g_rect_vgrad(g, 0, 0, S, S, 0, c3col(c3scale(c1, 0.35f), 1), c3col(c0, 1));
        Path p; path_init(&p); path_circle(&p, S / 2, S * 0.52f, S * 0.27f);
        Paint pt = {PAINT_LINEAR, c3col(c2, 1), c3col(c0, 1), 0, S * 0.25f, 0, S * 0.8f, 0};
        g_glow(g, S / 2, S * 0.52f, S * 0.5f, c3col(c0, 0.5f));
        path_fill(&s, &p, &pt); path_free(&p);
        for (int i = 0; i < 6; i++) g_rect(g, 0, S * (0.56f + i * 0.045f), S, S * (0.008f + i * 0.004f), 0, c3col(c3lerp(c3scale(c1, 0.35f), c0, 0.56f + i * 0.08f), 1));
        for (int i = 0; i < 9; i++) g_line(g, S / 2, S * 0.8f, S / 2 + (i - 4) * S * 0.3f, S, 2, c3col(c2, 0.6f));
        g_rect_vgrad(g, 0, S * 0.8f, S, S * 0.2f, 0, rgba(10, 0, 30, 200), rgba(10, 0, 30, 255));
        for (int i = 0; i < 9; i++) g_line(g, S / 2 + (i - 4) * S * 0.04f, S * 0.8f, S / 2 + (i - 4) * S * 0.3f, S, 2, c3col(c0, 0.8f));
        for (int i = 0; i < 4; i++) g_rect(g, 0, S * (0.82f + i * i * 0.012f), S, 2, 0, c3col(c0, 0.7f));
        break;
    }
    case 1: { /* soft gradient orbs */
        g_rect_grad(g, 0, 0, S, S, 0, c3col(c0, 1), c3col(c1, 1), 0, 0, S, S);
        for (int i = 0; i < 6; i++) g_glow(g, rng_f(&rg) * S, rng_f(&rg) * S, S * rng_range(&rg, 0.25f, 0.5f), c3col(i & 1 ? c2 : c3(1, 1, 1), 0.35f));
        g_rect(g, S * 0.18f, S * 0.18f, S * 0.64f, S * 0.64f, S * 0.32f, rgba(255, 255, 255, 40));
        g_stroke(g, S * 0.18f, S * 0.18f, S * 0.64f, S * 0.64f, S * 0.32f, 2, rgba(255, 255, 255, 120));
        break;
    }
    case 2: { /* glacier: concentric rings */
        g_rect_vgrad(g, 0, 0, S, S, 0, c3col(c2, 1), c3col(c3scale(c2, 0.4f), 1));
        for (int i = 18; i >= 1; i--) {
            float rr = i * S * 0.035f;
            g_stroke(g, S / 2 - rr, S * 0.45f - rr, rr * 2, rr * 2, rr, 2.5f, c3col(c3lerp(c0, c1, i / 18.f), 0.25f + 0.04f * (18 - i)));
        }
        g_circle(g, S / 2, S * 0.45f, S * 0.06f, c3col(c1, 1));
        g_glow(g, S / 2, S * 0.45f, S * 0.25f, c3col(c1, 0.6f));
        break;
    }
    case 3: { /* night market: grid of lanterns */
        g_rect(g, 0, 0, S, S, 0, c3col(c3scale(c2, 0.5f), 1));
        for (int y = 0; y < 6; y++)
            for (int x = 0; x < 6; x++) {
                float cx = S * (0.12f + x * 0.152f), cy = S * (0.12f + y * 0.152f);
                float rr = S * rng_range(&rg, 0.02f, 0.06f);
                C3 c = (x + y) & 1 ? c0 : c1;
                g_glow(g, cx, cy, rr * 3, c3col(c, 0.35f));
                g_circle(g, cx, cy, rr, c3col(c, 1));
            }
        break;
    }
    case 4: { /* piano: moon over still water */
        g_rect_vgrad(g, 0, 0, S, S * 0.62f, 0, c3col(c3scale(c2, 0.8f), 1), c3col(c1, 1));
        g_rect_vgrad(g, 0, S * 0.62f, S, S * 0.38f, 0, c3col(c3lerp(c1, c2, 0.5f), 1), c3col(c3scale(c2, 0.6f), 1));
        g_glow(g, S * 0.62f, S * 0.32f, S * 0.3f, c3col(c0, 0.5f));
        g_circle(g, S * 0.62f, S * 0.32f, S * 0.09f, c3col(c0, 1));
        for (int i = 0; i < 12; i++) g_rect(g, S * 0.62f - S * (0.03f + i * 0.012f), S * (0.66f + i * 0.026f), S * (0.06f + i * 0.024f), 3, 1.5f, c3col(c0, 0.6f - i * 0.04f));
        for (int i = 0; i < 8; i++) g_rect(g, S * 0.1f + i * S * 0.1f, S * 0.86f, S * 0.08f, S * 0.14f, 3, rgba(255, 255, 255, 200));
        for (int i = 0; i < 7; i++) if (i != 2 && i != 6) g_rect(g, S * 0.165f + i * S * 0.1f, S * 0.86f, S * 0.05f, S * 0.08f, 2, rgba(20, 20, 30, 230));
        break;
    }
    case 5: { /* chiptune pixels */
        g_rect(g, 0, 0, S, S, 0, c3col(c3scale(c2, 0.25f), 1));
        int P = 20;
        for (int y = 0; y < S / P; y++)
            for (int x = 0; x < S / P; x++) {
                float hx = x - 15, hy = y - 13;
                float heart = powf(hx * hx * 0.01f + hy * hy * 0.012f - 1, 3) - hx * hx * 0.01f * hy * hy * 0.012f * (-hy * 0.11f);
                C3 c = heart < 0 ? c2 : (rng_f(&rg) < 0.08f ? c1 : c3scale(c0, 0.15f + 0.1f * (y & 1)));
                g_rect(g, x * P + 1, y * P + 1, P - 2, P - 2, 2, c3col(c, 1));
            }
        break;
    }
    case 6: { /* cinematic: landscape miniature */
        SceneDesc sd = {SC_AURORA, TOD_MAGIC, EL_NONE, 900 + idx, 0.6f, 0.3f, 1, 0};
        scene_render(t, &sd, 0.3f, 0.6f, 15);
        g_rect_vgrad(g, 0, S * 0.6f, S, S * 0.4f, 0, rgba(0, 0, 0, 0), rgba(0, 0, 0, 140));
        break;
    }
    default: { /* funk: stripes and arcs */
        g_rect(g, 0, 0, S, S, 0, c3col(c3lerp(c0, c3(1, 1, 1), 0.7f), 1));
        for (int i = 0; i < 5; i++) {
            C3 c = i % 3 == 0 ? c0 : (i % 3 == 1 ? c1 : c2);
            Path p; path_init(&p);
            path_arc_stroke(&p, S * 0.5f, S * 1.05f, S * (0.25f + i * 0.09f), PI_F, 2 * PI_F, S * 0.07f);
            path_fill_color(&s, &p, c3col(c, 1));
            path_free(&p);
        }
        break;
    }
    }
    /* Typography */
    char tt[64];
    upper(tt, a->title, sizeof tt);
    int dark = a->style == 7;
    uint32_t tc = dark ? rgba(30, 20, 40, 255) : WHITE;
    g_text(g, FONT_BLACK, 40, 36, S - 84, tt, tc, ALIGN_LEFT, 2);
    g_text(g, FONT_MEDIUM, 20, 38, S - 34, a->artist, col_alpha(tc, 0.8f), ALIGN_LEFT, 1);
    gfx_free(g);
    /* grain */
    for (int i = 0; i < S * S; i++) {
        int n = (int)(hash32(i * 31 + idx) & 15) - 8;
        uint32_t c = t->px[i];
        int r = clampi((int)((c >> 16) & 255) + n, 0, 255), gg = clampi((int)((c >> 8) & 255) + n, 0, 255), b = clampi((int)(c & 255) + n, 0, 255);
        t->px[i] = 0xFF000000u | (r << 16) | (gg << 8) | b;
    }
    t->opaque = 1;
    tex_build_mips(t, 32);
    return t;
}

/* ======================= Wallpaper / grain / logos / map ======================= */
Texture *g_wallpaper, *g_wallpaper_blur, *g_grain, *g_weather_map;

static Texture *render_wallpaper(void) {
    const int W = SCREEN_W, H = SCREEN_H;
    Texture *t = tex_new(W, H);
    struct { float x, y, r; C3 c; } blobs[] = {
        {0.15f, 0.2f, 0.55f, c3hex(0xFF8A65)}, {0.85f, 0.15f, 0.6f, c3hex(0x7C4DFF)}, {0.6f, 0.85f, 0.6f, c3hex(0x00B8D4)},
        {0.1f, 0.9f, 0.5f, c3hex(0xFF4081)}, {0.5f, 0.4f, 0.4f, c3hex(0xFFD180)},
    };
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            float u = (float)x / W, v = (float)y / H;
            C3 c = c3lerp(c3hex(0x2B1E6E), c3hex(0x7A3E9D), v);
            for (int i = 0; i < 5; i++) {
                float dx = (u - blobs[i].x) * 1.6f, dy = v - blobs[i].y;
                float w = expf(-(dx * dx + dy * dy) / (blobs[i].r * blobs[i].r * 0.35f));
                c = c3lerp(c, blobs[i].c, w * 0.85f);
            }
            c = c3add(c, c3scale(c3(1, 1, 1), 0.05f));
            t->px[(size_t)y * W + x] = c3col(c, 1);
        }
    t->opaque = 1;
    return t;
}

static Texture *render_grain(void) {
    Texture *t = tex_new(256, 256);
    for (int i = 0; i < 256 * 256; i++) {
        uint32_t h = hash32(i * 7919 + 13);
        int v = (int)(h & 255);
        int a = (int)((h >> 8) & 63);
        t->px[i] = v > 128 ? rgba(255, 255, 255, a) : rgba(0, 0, 0, a);
    }
    return t;
}

static Texture *render_weather_map(void) {
    const int W = 960, H = 540;
    Texture *t = tex_new(W, H);
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            float u = x / 300.f, v = y / 300.f;
            float n = fbm2(u, v, 6, 4242) + 0.25f * (1 - fabsf((float)x / W - 0.5f) * 2) - 0.12f;
            C3 c;
            if (n > 0.52f) {
                float h = (n - 0.52f) * 4;
                c = c3lerp(c3hex(0x5FAF6A), c3hex(0xD8C79A), clampf(h, 0, 1));
                c = c3lerp(c, c3hex(0xF2F2F2), clampf(h - 0.9f, 0, 1));
                float shade = fbm2(u * 3, v * 3, 3, 99) * 0.25f;
                c = c3scale(c, 0.85f + shade);
            } else {
                float d = clampf((0.52f - n) * 5, 0, 1);
                c = c3lerp(c3hex(0x3BA7D6), c3hex(0x0E4E8C), d);
                if (n > 0.49f) c = c3lerp(c, c3hex(0x9BE1F5), (n - 0.49f) / 0.03f * 0.6f);
            }
            t->px[(size_t)y * W + x] = c3col(c, 1);
        }
    t->opaque = 1;
    return t;
}

static Texture *render_channel_logo(Channel *c) {
    const int W = 160, H = 96;
    Texture *t = tex_new(W, H);
    Surface s = tex_surface(t);
    Gfx *g = gfx_new_immediate(s);
    C3 col = c3hex(c->color & 0xFFFFFF);
    g_rect_grad(g, 0, 0, W, H, 0, c3col(c3lerp(col, c3(1, 1, 1), 0.2f), 1), c3col(c3scale(col, 0.6f), 1), 0, 0, W, H);
    g_glow(g, 20, 10, 90, rgba(255, 255, 255, 60));
    float size = fminf(40.f, (W - 24) / fmaxf(1.f, text_width(FONT_BLACK, 100, c->code, 1)) * 100.f);
    g_text_shadow(g, FONT_BLACK, size, W / 2.f, H / 2.f - font_cap(FONT_BLACK, size) / 2.f, c->code, WHITE, ALIGN_CENTER, 1);
    gfx_free(g);
    t->opaque = 1;
    tex_build_mips(t, 32);
    return t;
}

/* ======================= Scheduling ======================= */
static atomic_int g_done, g_total, g_ess_done, g_ess_total;

typedef struct { int kind, idx, essential; } AJob;
enum { AJ_WALL, AJ_AVATAR, AJ_APP, AJ_POSTER, AJ_BACKDROP, AJ_PHOTO, AJ_COVER, AJ_LOGO, AJ_MAP };

static void publish(Texture **slot, Texture *t) {
    t->ready = 1;
    __atomic_store_n(slot, t, __ATOMIC_RELEASE);
}

static void run_job(void *arg) {
    AJob *j = arg;
    switch (j->kind) {
    case AJ_WALL: {
        Texture *w = render_wallpaper();
        Texture *small = tex_downscale(w, 240, 135);
        tex_blur(small, 6);
        small->opaque = 1;
        publish(&g_wallpaper_blur, small);
        tex_build_mips(w, 64);
        publish(&g_wallpaper, w);
        break;
    }
    case AJ_AVATAR: publish(&g_profiles[j->idx].avatar, gen_avatar(j->idx)); break;
    case AJ_APP: publish(&g_apps[j->idx].tile, gen_app_tile(j->idx)); break;
    case AJ_POSTER: {
        Movie *m = &g_movies[j->idx];
        Texture *pt = render_poster(m);
        /* Blurred 16:9 crop of the artwork for cheap frosted backgrounds. */
        Texture *b = tex_new(240, 135);
        Gfx *bg = gfx_new_immediate(tex_surface(b));
        g_image_src(bg, pt, 0, pt->h * 0.3f, (float)pt->w, pt->w * 9.f / 16.f, 0, 0, 240, 135, 0, 1);
        gfx_free(bg);
        tex_blur(b, 5);
        b->opaque = 1;
        publish(&m->blur, b);
        publish(&m->poster, pt);
        break;
    }
    case AJ_BACKDROP: {
        Movie *m = &g_movies[j->idx];
        Texture *t = tex_new(SCREEN_W, SCREEN_H);
        scene_render(t, &m->scene, clampf(m->scene.sun_x - 0.95f, 0.f, 0.f), 1.7778f, 15);
        tex_build_mips(t, 64);
        Texture *thumb = t->mip && t->mip->mip ? t->mip->mip : t;
        publish(&m->thumb, thumb);
        publish(&m->backdrop, t);
        break;
    }
    case AJ_PHOTO: {
        Photo *p = &g_photos[j->idx];
        p->scene.photo = 1;
        Texture *t = tex_new(SCREEN_W, SCREEN_H);
        scene_render(t, &p->scene, 0, 1.7778f, 15);
        tex_build_mips(t, 64);
        Texture *b = tex_downscale(t, 240, 135);
        tex_blur(b, 4);
        b->opaque = 1;
        publish(&p->blur, b);
        publish(&p->thumb, t->mip && t->mip->mip ? t->mip->mip : t);
        publish(&p->image, t);
        break;
    }
    case AJ_COVER: {
        Album *a = &g_albums[j->idx];
        Texture *c = render_cover(a, j->idx);
        Texture *b = tex_downscale(c, 160, 160);
        tex_blur(b, 8);
        b->opaque = 1;
        publish(&a->cover_blur, b);
        publish(&a->cover, c);
        break;
    }
    case AJ_LOGO: publish(&g_channels[j->idx].logo, render_channel_logo(&g_channels[j->idx])); break;
    case AJ_MAP: publish(&g_weather_map, render_weather_map()); break;
    }
    if (j->essential) atomic_fetch_add(&g_ess_done, 1);
    atomic_fetch_add(&g_done, 1);
    free(j);
}

static void q(int kind, int idx, int prio, int essential) {
    AJob *j = xmalloc(sizeof *j);
    j->kind = kind; j->idx = idx; j->essential = essential;
    atomic_fetch_add(&g_total, 1);
    if (essential) atomic_fetch_add(&g_ess_total, 1);
    job_submit(run_job, j, prio);
}

void assets_start(void) {
    g_grain = render_grain();
    g_grain->ready = 1;
    q(AJ_WALL, 0, 100, 1);
    for (int i = 0; i < g_nprofiles; i++) q(AJ_AVATAR, i, 99, 1);
    for (int i = 0; i < g_napps; i++) q(AJ_APP, i, 90, 1);
    for (int i = 0; i < g_nfeatured; i++) q(AJ_BACKDROP, g_featured[i], 85, 1);
    for (int i = 0; i < g_nmovies; i++) q(AJ_POSTER, i, 80, 1);
    for (int i = 0; i < g_ncontinue; i++) {
        int dup = 0;
        for (int k = 0; k < g_nfeatured; k++) dup |= g_featured[k] == g_continue[i].movie;
        if (!dup) q(AJ_BACKDROP, g_continue[i].movie, 70, 1);
    }
    for (int i = 0; i < g_nchannels; i++) q(AJ_LOGO, i, 60, 0);
    for (int i = 0; i < g_nalbums; i++) q(AJ_COVER, i, 55, 0);
    q(AJ_MAP, 0, 50, 0);
    for (int i = 0; i < g_nmovies; i++) {
        int dup = 0;
        for (int k = 0; k < g_nfeatured; k++) dup |= g_featured[k] == i;
        for (int k = 0; k < g_ncontinue; k++) dup |= g_continue[k].movie == i;
        if (!dup) q(AJ_BACKDROP, i, 40, 0);
    }
    for (int i = 0; i < g_nphotos; i++) q(AJ_PHOTO, i, 30, 0);
}

float assets_progress(void) {
    int t = atomic_load(&g_total);
    return t ? (float)atomic_load(&g_done) / t : 0.f;
}
int assets_essential_ready(void) { return atomic_load(&g_ess_done) >= atomic_load(&g_ess_total); }

/* Test hooks */
Texture *render_poster_pub(Movie *m) { return render_poster(m); }
Texture *render_cover_pub(Album *a, int i) { return render_cover(a, i); }
