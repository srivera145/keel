/* KEEL OS — core types, math, color, random, noise. */
#pragma once
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCREEN_W 1920
#define SCREEN_H 1080

#define MINI(a, b) ((a) < (b) ? (a) : (b))
#define MAXI(a, b) ((a) > (b) ? (a) : (b))
#define COUNT(a) ((int)(sizeof(a) / sizeof((a)[0])))
#define PI_F 3.14159265358979f

static inline float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
static inline int clampi(int v, int a, int b) { return v < a ? a : (v > b ? b : v); }
static inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
static inline float sat(float v) { return clampf(v, 0.f, 1.f); }
static inline float smoothstep(float a, float b, float x) {
    float t = sat((x - a) / (b - a));
    return t * t * (3.f - 2.f * t);
}
/* Frame-rate independent exponential approach. */
static inline float approach(float cur, float target, float dt, float rate) {
    return target + (cur - target) * expf(-rate * dt);
}
static inline float ease_out_cubic(float t) { t = sat(t); float u = 1.f - t; return 1.f - u * u * u; }
static inline float ease_in_out_cubic(float t) {
    t = sat(t);
    return t < 0.5f ? 4.f * t * t * t : 1.f - powf(-2.f * t + 2.f, 3.f) * 0.5f;
}
static inline float ease_out_back(float t) {
    t = sat(t);
    const float c1 = 1.70158f, c3 = c1 + 1.f;
    float u = t - 1.f;
    return 1.f + c3 * u * u * u + c1 * u * u;
}

/* ---------- Color: 32-bit premultiplied ARGB ---------- */
static inline uint32_t rgba(int r, int g, int b, int a) {
    r = clampi(r, 0, 255); g = clampi(g, 0, 255); b = clampi(b, 0, 255); a = clampi(a, 0, 255);
    return ((uint32_t)a << 24) | ((uint32_t)(r * a / 255) << 16) | ((uint32_t)(g * a / 255) << 8) | (uint32_t)(b * a / 255);
}
#define RGB(r, g, b) (0xFF000000u | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))
#define HEX(h) (0xFF000000u | (uint32_t)(h))
#define WHITE 0xFFFFFFFFu
#define BLACK 0xFF000000u
#define CLEAR 0x00000000u

static inline int col_a(uint32_t c) { return (int)(c >> 24); }
static inline int col_r(uint32_t c) { return (int)((c >> 16) & 255); }
static inline int col_g(uint32_t c) { return (int)((c >> 8) & 255); }
static inline int col_b(uint32_t c) { return (int)(c & 255); }

/* Scale a premultiplied color by an 8-bit factor. */
static inline uint32_t mul8(uint32_t c, uint32_t a) {
    uint32_t rb = (c & 0x00FF00FFu) * a;
    uint32_t ag = ((c >> 8) & 0x00FF00FFu) * a;
    rb = ((rb + ((rb >> 8) & 0x00FF00FFu) + 0x00800080u) >> 8) & 0x00FF00FFu;
    ag = (ag + ((ag >> 8) & 0x00FF00FFu) + 0x00800080u) & 0xFF00FF00u;
    return rb | ag;
}
/* Source-over for premultiplied colors. */
static inline uint32_t blend(uint32_t d, uint32_t s) {
    return s + mul8(d, 255u - (s >> 24));
}
static inline uint32_t col_alpha(uint32_t c, float a) {
    if (a >= 1.f) return c;
    if (a <= 0.f) return 0;
    return mul8(c, (uint32_t)(a * 255.f + 0.5f));
}
/* Lerp premultiplied colors with t in [0,256]. */
static inline uint32_t lerp256(uint32_t a, uint32_t b, uint32_t t) {
    return mul8(a, 255 - (t > 255 ? 255 : t)) + mul8(b, t > 255 ? 255 : t);
}
static inline uint32_t col_lerp(uint32_t a, uint32_t b, float t) {
    return lerp256(a, b, (uint32_t)(sat(t) * 255.f + 0.5f));
}
/* Straight (non-premult) helpers for art generation. */
typedef struct { float r, g, b; } C3;
static inline C3 c3(float r, float g, float b) { C3 c = {r, g, b}; return c; }
static inline C3 c3hex(uint32_t h) { return c3(((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f); }
static inline C3 c3lerp(C3 a, C3 b, float t) { return c3(lerpf(a.r, b.r, t), lerpf(a.g, b.g, t), lerpf(a.b, b.b, t)); }
static inline C3 c3scale(C3 a, float s) { return c3(a.r * s, a.g * s, a.b * s); }
static inline C3 c3add(C3 a, C3 b) { return c3(a.r + b.r, a.g + b.g, a.b + b.b); }
static inline uint32_t c3col(C3 c, float a) {
    return rgba((int)(sat(c.r) * 255.f + .5f), (int)(sat(c.g) * 255.f + .5f), (int)(sat(c.b) * 255.f + .5f), (int)(sat(a) * 255.f + .5f));
}
static inline uint32_t hsv(float h, float s, float v, float a) {
    h = fmodf(h, 1.f); if (h < 0) h += 1.f;
    float r, g, b, f = h * 6.f, p, q, t; int i = (int)f; f -= i;
    p = v * (1 - s); q = v * (1 - s * f); t = v * (1 - s * (1 - f));
    switch (i % 6) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    default: r = v; g = p; b = q; break;
    }
    return c3col(c3(r, g, b), a);
}

/* ---------- Random & noise ---------- */
static inline uint32_t hash32(uint32_t x) {
    x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16;
    return x;
}
static inline uint32_t hash2i(int x, int y, uint32_t seed) {
    return hash32((uint32_t)x * 0x9E3779B1u ^ hash32((uint32_t)y * 0x85EBCA77u ^ seed));
}
static inline float hashf(uint32_t x) { return (hash32(x) & 0xFFFFFF) / 16777216.f; }

typedef struct { uint64_t s; } Rng;
static inline Rng rng_make(uint64_t seed) { Rng r = {seed * 0x9E3779B97F4A7C15ull + 0x632BE59BD9B4E019ull}; return r; }
static inline uint32_t rng_u32(Rng *r) {
    r->s = r->s * 6364136223846793005ull + 1442695040888963407ull;
    uint32_t x = (uint32_t)(r->s >> 33) ^ (uint32_t)(r->s >> 13);
    return hash32(x);
}
static inline float rng_f(Rng *r) { return (rng_u32(r) & 0xFFFFFF) / 16777216.f; }
static inline float rng_range(Rng *r, float a, float b) { return a + (b - a) * rng_f(r); }
static inline int rng_int(Rng *r, int n) { return n <= 0 ? 0 : (int)(rng_u32(r) % (uint32_t)n); }

static inline float vnoise1(float x, uint32_t seed) {
    int i = (int)floorf(x); float f = x - i;
    float u = f * f * (3 - 2 * f);
    return lerpf(hashf((uint32_t)i ^ seed), hashf((uint32_t)(i + 1) ^ seed), u);
}
static inline float fbm1(float x, int oct, uint32_t seed) {
    float a = 0.5f, s = 0, n = 0;
    for (int i = 0; i < oct; i++) { s += a * vnoise1(x, seed + i * 977u); n += a; x *= 2.03f; a *= 0.5f; }
    return s / n;
}
static inline float vnoise2(float x, float y, uint32_t seed) {
    int ix = (int)floorf(x), iy = (int)floorf(y);
    float fx = x - ix, fy = y - iy;
    float ux = fx * fx * (3 - 2 * fx), uy = fy * fy * (3 - 2 * fy);
    float a = (hash2i(ix, iy, seed) & 0xFFFF) / 65535.f;
    float b = (hash2i(ix + 1, iy, seed) & 0xFFFF) / 65535.f;
    float c = (hash2i(ix, iy + 1, seed) & 0xFFFF) / 65535.f;
    float d = (hash2i(ix + 1, iy + 1, seed) & 0xFFFF) / 65535.f;
    return lerpf(lerpf(a, b, ux), lerpf(c, d, ux), uy);
}
static inline float fbm2(float x, float y, int oct, uint32_t seed) {
    float a = 0.5f, s = 0, n = 0;
    for (int i = 0; i < oct; i++) {
        s += a * vnoise2(x, y, seed + i * 7919u); n += a;
        float nx = x * 1.6f - y * 1.2f, ny = x * 1.2f + y * 1.6f; x = nx; y = ny;
        a *= 0.5f;
    }
    return s / n;
}

double now_sec(void);
void *xmalloc(size_t n);
void *xcalloc(size_t n, size_t s);
