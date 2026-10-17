/* KEEL OS — software compositor.
 *
 * Frames are recorded into a display list, then executed in parallel: the
 * screen is cut into horizontal bands and each render thread draws every
 * command clipped to the bands it picks up. Frosted-glass backdrops act as
 * barriers: everything recorded before them is finished, the region under
 * the glass is downsampled 4x, blurred (3 box passes ~ gaussian), tinted
 * and saturated, then composited back with rounded, anti-aliased edges.
 */
#include "gfx.h"
#include "jobs.h"

enum { C_RRECT = 1, C_IMAGE, C_MASK, C_GLYPHS, C_CAPSULE, C_POLY, C_TILE, C_BLUR, C_GLOW };

typedef struct {
    float x0, y0, x1, y1, r, f, stroke;
    uint32_t c0, c1;
    float gx, gy, gdx, gdy;
} RRCmd;
typedef struct {
    const Texture *t;
    float x0, y0, x1, y1;
    float sx0, sy0, sx1, sy1;
    float r;
    uint32_t alpha;
} ImgCmd;
typedef struct {
    const Mask *m;
    float x0, y0, x1, y1;
    uint32_t c;
} MaskCmd;
typedef struct { const GlyphPos *g; int n; uint32_t c; } TxtCmd;
typedef struct { float ax, ay, bx, by, r; uint32_t c; } CapCmd;
typedef struct { float nx[8], ny[8], nc[8]; int n; uint32_t c; } PolyCmd;
typedef struct { const Texture *t; int ox, oy; uint32_t alpha; } TileCmd;
typedef struct { int slot; } BlurCmd;
typedef struct { float cx, cy, r; uint32_t c; } GlowCmd;

typedef struct {
    int type;
    IRect clip;
    union {
        RRCmd rr;
        ImgCmd img;
        MaskCmd msk;
        TxtCmd txt;
        CapCmd cap;
        PolyCmd poly;
        TileCmd tile;
        BlurCmd blur;
        GlowCmd glow;
    };
} Cmd;

typedef struct {
    IRect src;      /* full-resolution region captured (multiple of 4) */
    int capture;    /* 1: plain full-res copy (layers), 0: 4x-down blur */
    int lw, lh, radius;
    uint32_t tint;
    float sat;
    uint32_t *buf;
    size_t cap;
    Texture tex;
} Slot;

typedef struct { float tx, ty, alpha; IRect clip; } GState;

typedef struct Chunk { struct Chunk *next; size_t used, cap; uint8_t data[]; } Chunk;

struct Gfx {
    int immediate;
    Surface target;
    Cmd *cmds;
    int ncmds, capcmds;
    Chunk *chunks, *cur;
    GState st[48];
    int sp;
    Slot **slots;
    int nslots, capslots;
};

int g_stat_blur_regions;

/* ======================= Textures ======================= */
Texture *tex_new(int w, int h) {
    Texture *t = xcalloc(1, sizeof *t);
    t->w = w > 0 ? w : 1;
    t->h = h > 0 ? h : 1;
    t->px = xcalloc((size_t)t->w * t->h, 4);
    return t;
}
void tex_free(Texture *t) {
    if (!t) return;
    tex_free(t->mip);
    free(t->px);
    free(t);
}
Surface tex_surface(Texture *t) {
    Surface s = {t->px, t->w, t->h, t->w};
    return s;
}
void tex_clear(Texture *t, uint32_t c) {
    size_t n = (size_t)t->w * t->h;
    for (size_t i = 0; i < n; i++) t->px[i] = c;
}
void tex_mark_opaque(Texture *t) {
    size_t n = (size_t)t->w * t->h;
    for (size_t i = 0; i < n; i++) t->px[i] |= 0xFF000000u;
    t->opaque = 1;
}
static uint32_t avg4(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    uint32_t rb = (a & 0x00FF00FF) + (b & 0x00FF00FF) + (c & 0x00FF00FF) + (d & 0x00FF00FF) + 0x00020002;
    uint32_t ag = ((a >> 8) & 0x00FF00FF) + ((b >> 8) & 0x00FF00FF) + ((c >> 8) & 0x00FF00FF) + ((d >> 8) & 0x00FF00FF) + 0x00020002;
    return ((rb >> 2) & 0x00FF00FF) | (((ag >> 2) & 0x00FF00FF) << 8);
}
void tex_build_mips(Texture *t, int min_size) {
    Texture *p = t;
    while (p->w / 2 >= min_size && p->h / 2 >= min_size && !p->mip) {
        Texture *m = tex_new(p->w / 2, p->h / 2);
        m->opaque = p->opaque;
        for (int y = 0; y < m->h; y++) {
            const uint32_t *r0 = p->px + (size_t)(2 * y) * p->w, *r1 = r0 + p->w;
            uint32_t *o = m->px + (size_t)y * m->w;
            for (int x = 0; x < m->w; x++) o[x] = avg4(r0[2 * x], r0[2 * x + 1], r1[2 * x], r1[2 * x + 1]);
        }
        m->ready = 1;
        p->mip = m;
        p = m;
    }
}
Texture *tex_downscale(const Texture *src, int w, int h) {
    Texture *t = tex_new(w, h);
    t->opaque = src->opaque;
    float sx = (float)src->w / w, sy = (float)src->h / h;
    for (int y = 0; y < h; y++) {
        int y0 = (int)(y * sy), y1 = MAXI(y0 + 1, (int)((y + 1) * sy));
        if (y1 > src->h) y1 = src->h;
        for (int x = 0; x < w; x++) {
            int x0 = (int)(x * sx), x1 = MAXI(x0 + 1, (int)((x + 1) * sx));
            if (x1 > src->w) x1 = src->w;
            uint32_t a = 0, r = 0, g = 0, b = 0, n = 0;
            for (int yy = y0; yy < y1; yy++) {
                const uint32_t *row = src->px + (size_t)yy * src->w;
                for (int xx = x0; xx < x1; xx++) {
                    uint32_t c = row[xx];
                    a += c >> 24; r += (c >> 16) & 255; g += (c >> 8) & 255; b += c & 255; n++;
                }
            }
            t->px[(size_t)y * w + x] = ((a / n) << 24) | ((r / n) << 16) | ((g / n) << 8) | (b / n);
        }
    }
    return t;
}

/* Running-sum box blur of 4 interleaved channels over n samples with stride. */
static void box_line(uint32_t *p, int n, int stride, int r, int *tmp) {
    if (n <= 1 || r <= 0) return;
    int *ch = tmp; /* 4*n ints */
    for (int i = 0; i < n; i++) {
        uint32_t c = p[(size_t)i * stride];
        ch[i] = c >> 24; ch[n + i] = (c >> 16) & 255; ch[2 * n + i] = (c >> 8) & 255; ch[3 * n + i] = c & 255;
    }
    int *out = tmp + 4 * n;
    int div = 2 * r + 1, inv = (1 << 16) / div;
    for (int pass = 0; pass < 3; pass++) {
        for (int k = 0; k < 4; k++) {
            int *in = ch + k * n, *o = out + k * n;
            int s = in[0] * (r + 1);
            for (int i = 1; i <= r; i++) s += in[MINI(i, n - 1)];
            for (int i = 0; i < n; i++) {
                o[i] = (s * inv) >> 16;
                s += in[MINI(i + r + 1, n - 1)] - in[MAXI(i - r, 0)];
            }
        }
        int *sw = ch; ch = out; out = sw;
    }
    for (int i = 0; i < n; i++)
        p[(size_t)i * stride] = ((uint32_t)ch[i] << 24) | ((uint32_t)ch[n + i] << 16) | ((uint32_t)ch[2 * n + i] << 8) | (uint32_t)ch[3 * n + i];
}
void tex_blur(Texture *t, int radius) {
    int m = MAXI(t->w, t->h);
    int *tmp = xmalloc(sizeof(int) * 8 * m);
    for (int y = 0; y < t->h; y++) box_line(t->px + (size_t)y * t->w, t->w, 1, radius, tmp);
    for (int x = 0; x < t->w; x++) box_line(t->px + x, t->h, t->w, radius, tmp);
    free(tmp);
}

void surf_fill(Surface *s, uint32_t c) {
    for (int y = 0; y < s->h; y++) {
        uint32_t *row = s->px + (size_t)y * s->stride;
        for (int x = 0; x < s->w; x++) row[x] = c;
    }
}

/* ======================= Shapes ======================= */
typedef struct { float x0, y0, x1, y1, r, f, cx, cy, hw, hh, invf; int soft; } RRShape;

static inline void rr_init(RRShape *s, float x0, float y0, float x1, float y1, float r, float f) {
    s->x0 = x0; s->y0 = y0; s->x1 = x1; s->y1 = y1;
    float mr = fminf(x1 - x0, y1 - y0) * 0.5f;
    s->r = clampf(r, 0.f, mr);
    s->f = f < 1.f ? 1.f : f;
    s->invf = 1.f / s->f;
    s->soft = s->f > 1.5f;
    s->cx = (x0 + x1) * 0.5f; s->cy = (y0 + y1) * 0.5f;
    s->hw = (x1 - x0) * 0.5f - s->r; s->hh = (y1 - y0) * 0.5f - s->r;
}
static inline float rr_sdf(const RRShape *s, float px, float py) {
    float qx = fabsf(px - s->cx) - s->hw, qy = fabsf(py - s->cy) - s->hh;
    float ox = qx > 0 ? qx : 0, oy = qy > 0 ? qy : 0;
    float in = qx > qy ? qx : qy;
    return sqrtf(ox * ox + oy * oy) + (in < 0 ? in : 0) - s->r;
}
static inline uint32_t cov_from_d(const RRShape *s, float d) {
    float c = 0.5f - d * s->invf;
    if (c <= 0) return 0;
    if (c >= 1) return 255;
    if (s->soft) c = c * c * (3 - 2 * c);
    return (uint32_t)(c * 255.f + 0.5f);
}
/* For row y, compute [a0,a1) touched pixels and [f0,f1) fully covered interior. */
static inline int rr_row(const RRShape *s, int y, int cx0, int cx1, int *a0, int *f0, int *f1, int *a1) {
    float yc = y + 0.5f;
    if (yc < s->y0 - s->f || yc > s->y1 + s->f) return 0;
    int lo = MAXI(cx0, (int)floorf(s->x0 - s->f));
    int hi = MINI(cx1, (int)ceilf(s->x1 + s->f));
    if (lo >= hi) return 0;
    float band = fmaxf(s->r, s->f) + 1.f;
    int fl = hi, fh = hi;
    if (yc >= s->y0 + band && yc <= s->y1 - band) {
        fl = (int)ceilf(s->x0 + s->f + 1.f);
        fh = (int)floorf(s->x1 - s->f - 1.f);
        if (fl < lo) fl = lo;
        if (fh > hi) fh = hi;
        if (fh <= fl) fl = fh = hi;
    }
    *a0 = lo; *a1 = hi; *f0 = fl; *f1 = fh;
    return 1;
}

static inline void span_fill(uint32_t *d, int n, uint32_t c) {
    if ((c >> 24) == 255) {
        for (int i = 0; i < n; i++) d[i] = c;
    } else if (c) {
        uint32_t ia = 255u - (c >> 24);
        for (int i = 0; i < n; i++) d[i] = c + mul8(d[i], ia);
    }
}
static inline void put_cov(uint32_t *d, uint32_t c, uint32_t cov) {
    if (cov >= 255) *d = ((c >> 24) == 255) ? c : blend(*d, c);
    else if (cov) *d = blend(*d, mul8(c, cov));
}

static void exec_rrect(Surface *s, IRect c, const RRCmd *p) {
    RRShape sh;
    rr_init(&sh, p->x0, p->y0, p->x1, p->y1, p->r, p->f);
    int grad = p->c0 != p->c1;
    float sw = p->stroke;
    uint32_t *lut = NULL;
    for (int y = c.y0; y < c.y1; y++) {
        int a0, f0, f1, a1;
        if (!rr_row(&sh, y, c.x0, c.x1, &a0, &f0, &f1, &a1)) continue;
        uint32_t *row = s->px + (size_t)y * s->stride;
        float py = y + 0.5f;
        if (sw > 0) {
            /* Ring: interior of the inset shape is empty. */
            int il = f0, ih = f1;
            if (f1 > f0) { il = MAXI(f0, (int)ceilf(p->x0 + sw + sh.f + 1)); ih = MINI(f1, (int)floorf(p->x1 - sw - sh.f - 1)); if (ih < il) il = ih = f0; }
            else { il = ih = a1; }
            for (int x = a0; x < a1; x++) {
                if (x == il && ih > il) { x = ih - 1; continue; }
                float d = rr_sdf(&sh, x + 0.5f, py);
                d = fabsf(d + sw * 0.5f) - sw * 0.5f;
                uint32_t cov = cov_from_d(&sh, d);
                if (!cov) continue;
                uint32_t col = p->c0;
                if (grad) col = lerp256(p->c0, p->c1, (uint32_t)(sat((x + 0.5f - p->gx) * p->gdx + (py - p->gy) * p->gdy) * 256.f));
                put_cov(row + x, col, cov);
            }
            continue;
        }
        if (!grad) {
            for (int x = a0; x < f0; x++) put_cov(row + x, p->c0, cov_from_d(&sh, rr_sdf(&sh, x + 0.5f, py)));
            span_fill(row + f0, f1 - f0, p->c0);
            for (int x = f1; x < a1; x++) put_cov(row + x, p->c0, cov_from_d(&sh, rr_sdf(&sh, x + 0.5f, py)));
        } else if (p->gdx == 0) {
            /* vertical gradient: one color per row */
            uint32_t col = lerp256(p->c0, p->c1, (uint32_t)(sat((py - p->gy) * p->gdy) * 256.f));
            for (int x = a0; x < f0; x++) put_cov(row + x, col, cov_from_d(&sh, rr_sdf(&sh, x + 0.5f, py)));
            span_fill(row + f0, f1 - f0, col);
            for (int x = f1; x < a1; x++) put_cov(row + x, col, cov_from_d(&sh, rr_sdf(&sh, x + 0.5f, py)));
        } else {
            if (p->gdy == 0 && !lut) {
                /* horizontal gradient: same colors on every row, build once */
                lut = xmalloc(sizeof(uint32_t) * (c.x1 - c.x0));
                for (int x = c.x0; x < c.x1; x++) lut[x - c.x0] = lerp256(p->c0, p->c1, (uint32_t)(sat((x + 0.5f - p->gx) * p->gdx) * 256.f));
            }
            float tb = (a0 + 0.5f - p->gx) * p->gdx + (py - p->gy) * p->gdy;
            for (int x = a0; x < a1; x++) {
                uint32_t col = lut ? lut[x - c.x0] : lerp256(p->c0, p->c1, (uint32_t)(sat(tb + (x - a0) * p->gdx) * 256.f));
                uint32_t cov = (x >= f0 && x < f1) ? 255 : cov_from_d(&sh, rr_sdf(&sh, x + 0.5f, py));
                put_cov(row + x, col, cov);
            }
        }
    }
    free(lut);
}

static inline uint32_t tex_at(const Texture *t, int x, int y) {
    x = x < 0 ? 0 : (x >= t->w ? t->w - 1 : x);
    y = y < 0 ? 0 : (y >= t->h ? t->h - 1 : y);
    return t->px[(size_t)y * t->w + x];
}

/* Bilinear blend of 4 premultiplied texels, weights in 1/256 (sum 256). */
static inline uint32_t bilerp(uint32_t p00, uint32_t p01, uint32_t p10, uint32_t p11, uint32_t fx, uint32_t fy) {
    uint32_t w11 = (fx * fy + 128) >> 8, w01 = fx - w11, w10 = fy - w11, w00 = 256 - fx - fy + w11;
    const uint32_t M = 0x00FF00FFu;
    uint32_t rb = (((p00 & M) * w00 + (p01 & M) * w01 + (p10 & M) * w10 + (p11 & M) * w11) >> 8) & M;
    uint32_t ag = (((p00 >> 8) & M) * w00 + ((p01 >> 8) & M) * w01 + ((p10 >> 8) & M) * w10 + ((p11 >> 8) & M) * w11) & ~M;
    return rb | ag;
}

/* ---- 2-pixel SIMD bilinear (GCC vector extensions: SSE2 on x86, NEON on ARM) ---- */
typedef uint16_t v8u16 __attribute__((vector_size(16)));
typedef uint8_t v8u8 __attribute__((vector_size(8)));
static inline v8u16 ld2(uint32_t a, uint32_t b) {
    uint64_t q = (uint64_t)a | ((uint64_t)b << 32);
    v8u8 v;
    memcpy(&v, &q, 8);
    return __builtin_convertvector(v, v8u16);
}
static inline void st2(uint32_t *d, v8u16 v) {
    v8u8 r = __builtin_convertvector(v, v8u8);
    memcpy(d, &r, 8);
}
/* mode 0: store (opaque), 1: blend premultiplied, 2: blend scaled by alpha */
static void bilerp_span(uint32_t *dp, int n, const uint32_t *r0, const uint32_t *r1, int32_t u16, int32_t du16, uint32_t fy, int mode, uint32_t alpha) {
    const v8u16 FY = {fy, fy, fy, fy, fy, fy, fy, fy};
    const v8u16 K256 = {256, 256, 256, 256, 256, 256, 256, 256}, K128 = {128, 128, 128, 128, 128, 128, 128, 128};
    const v8u16 K255 = {255, 255, 255, 255, 255, 255, 255, 255};
    const v8u16 AL = {alpha, alpha, alpha, alpha, alpha, alpha, alpha, alpha};
    int i = 0;
    for (; i + 1 < n; i += 2) {
        int ua = u16 >> 16, ub = (u16 + du16) >> 16;
        uint16_t fa = (uint16_t)((u16 >> 8) & 255), fb = (uint16_t)(((u16 + du16) >> 8) & 255);
        u16 += 2 * du16;
        v8u16 FX = {fa, fa, fa, fa, fb, fb, fb, fb};
        v8u16 W11 = (FX * FY + K128) >> 8;
        v8u16 W01 = FX - W11, W10 = FY - W11, W00 = K256 - FX - FY + W11;
        v8u16 A = ld2(r0[ua], r0[ub]), B = ld2(r0[ua + 1], r0[ub + 1]);
        v8u16 C = ld2(r1[ua], r1[ub]), D = ld2(r1[ua + 1], r1[ub + 1]);
        v8u16 px = (A * W00 + B * W01 + C * W10 + D * W11) >> 8;
        if (mode == 0) { st2(dp + i, px); continue; }
        if (mode == 2) { v8u16 t = px * AL + K128; px = (t + (t >> 8)) >> 8; }
        v8u16 dst = ld2(dp[i], dp[i + 1]);
        v8u16 sa = __builtin_shuffle(px, (v8u16){3, 3, 3, 3, 7, 7, 7, 7});
        v8u16 t = dst * (K255 - sa) + K128;
        st2(dp + i, px + ((t + (t >> 8)) >> 8));
    }
    for (; i < n; i++, u16 += du16) {
        int ui = u16 >> 16;
        uint32_t fx = (u16 >> 8) & 255;
        uint32_t v = bilerp(r0[ui], r0[ui + 1], r1[ui], r1[ui + 1], fx, fy);
        if (mode == 0) dp[i] = v;
        else if (mode == 1) dp[i] = blend(dp[i], v);
        else dp[i] = blend(dp[i], mul8(v, alpha));
    }
}

static void exec_image(Surface *s, IRect c, const ImgCmd *p) {
    const Texture *t = p->t;
    if (!t || !t->px) return;
    RRShape sh;
    rr_init(&sh, p->x0, p->y0, p->x1, p->y1, p->r, 1.f);
    float du = (p->sx1 - p->sx0) / (p->x1 - p->x0);
    float dv = (p->sy1 - p->sy0) / (p->y1 - p->y0);
    uint32_t alpha = p->alpha;
    int unit = fabsf(du - 1.f) < 1e-4f && fabsf(dv - 1.f) < 1e-4f;
    float offu = p->sx0 - p->x0, offv = p->sy0 - p->y0;
    int nearest = 0, iu = 0, iv = 0;
    if (unit) {
        float ru = roundf(offu), rv = roundf(offv);
        if (fabsf(offu - ru) < 1.f / 256 && fabsf(offv - rv) < 1.f / 256) { nearest = 1; iu = (int)ru; iv = (int)rv; }
    }
    const int direct = t->opaque && alpha == 255;
    int32_t du16 = (int32_t)(du * 65536.f);
    const int wmax = t->w - 1;
    for (int y = c.y0; y < c.y1; y++) {
        int a0, f0, f1, a1;
        if (!rr_row(&sh, y, c.x0, c.x1, &a0, &f0, &f1, &a1)) continue;
        uint32_t *row = s->px + (size_t)y * s->stride;
        float py = y + 0.5f;
        if (nearest) {
            int ty = clampi(y + iv, 0, t->h - 1);
            const uint32_t *src = t->px + (size_t)ty * t->w;
            /* interior span: straight copy or blend */
            int lo = MAXI(f0, -iu), hi = MINI(f1, t->w - iu);
            if (hi > lo) {
                const uint32_t *sp = src + lo + iu;
                uint32_t *dp = row + lo;
                int n = hi - lo;
                if (direct) memcpy(dp, sp, (size_t)n * 4);
                else if (alpha == 255) for (int i = 0; i < n; i++) dp[i] = blend(dp[i], sp[i]);
                else for (int i = 0; i < n; i++) dp[i] = blend(dp[i], mul8(sp[i], alpha));
            } else lo = hi = f0;
            for (int x = a0; x < a1; x++) {
                if (x == lo && hi > lo) { x = hi - 1; continue; }
                uint32_t cov = (x >= f0 && x < f1) ? 255 : cov_from_d(&sh, rr_sdf(&sh, x + 0.5f, py));
                if (!cov) continue;
                int tx = x + iu;
                uint32_t px = src[tx < 0 ? 0 : (tx > wmax ? wmax : tx)];
                uint32_t a = cov == 255 ? alpha : (alpha * cov + 127) / 255;
                if (a == 255) row[x] = t->opaque ? px : blend(row[x], px);
                else row[x] = blend(row[x], mul8(px, a));
            }
            continue;
        }
        float v = p->sy0 + (py - p->y0) * dv - 0.5f;
        if (v + 1.f < t->ytop) continue;
        int vi = (int)floorf(v);
        uint32_t fy = (uint32_t)((v - vi) * 256.f);
        if (vi < 0) { vi = 0; fy = 0; }
        if (vi >= t->h - 1) { vi = t->h - 1; fy = 0; }
        const uint32_t *r0 = t->px + (size_t)vi * t->w, *r1 = t->px + (size_t)MINI(vi + 1, t->h - 1) * t->w;
        int32_t ubase = (int32_t)((p->sx0 + (a0 + 0.5f - p->x0) * du - 0.5f) * 65536.f);
        /* Interior [f0,f1): full coverage; find the sub-span needing no clamps. */
        int s0 = f0, s1 = f1;
        if (s1 > s0) {
            int32_t ua = ubase + (s0 - a0) * du16;
            while (s0 < s1 && (ua >> 16) < 0) { s0++; ua += du16; }
            int32_t ub = ubase + (s1 - 1 - a0) * du16;
            while (s1 > s0 && (ub >> 16) + 1 > wmax) { s1--; ub -= du16; }
            int32_t u16 = ubase + (s0 - a0) * du16;
            bilerp_span(row + s0, s1 - s0, r0, r1, u16, du16, fy, direct ? 0 : (alpha == 255 ? 1 : 2), alpha);
        } else s0 = s1 = f0;
        /* Edges and clamped texels. */
        int32_t u16 = ubase;
        for (int x = a0; x < a1; x++, u16 += du16) {
            if (x == s0 && s1 > s0) { u16 += (s1 - s0 - 1) * du16; x = s1 - 1; continue; }
            uint32_t cov = (x >= f0 && x < f1) ? 255 : cov_from_d(&sh, rr_sdf(&sh, x + 0.5f, py));
            if (!cov) continue;
            int ui = u16 >> 16;
            uint32_t fx = (u16 >> 8) & 255;
            int ua = ui < 0 ? 0 : (ui > wmax ? wmax : ui);
            int ub = ui + 1 < 0 ? 0 : (ui + 1 > wmax ? wmax : ui + 1);
            uint32_t px = bilerp(r0[ua], r0[ub], r1[ua], r1[ub], fx, fy);
            uint32_t a = cov == 255 ? alpha : (alpha * cov + 127) / 255;
            if (a == 255) row[x] = t->opaque ? px : blend(row[x], px);
            else row[x] = blend(row[x], mul8(px, a));
        }
    }
}

static void exec_mask(Surface *s, IRect c, const MaskCmd *p) {
    const Mask *m = p->m;
    float du = m->w / (p->x1 - p->x0), dv = m->h / (p->y1 - p->y0);
    for (int y = c.y0; y < c.y1; y++) {
        float v = (y + 0.5f - p->y0) * dv - 0.5f;
        int vi = (int)floorf(v);
        uint32_t fy = (uint32_t)((v - vi) * 256.f);
        int v0 = clampi(vi, 0, m->h - 1), v1 = clampi(vi + 1, 0, m->h - 1);
        if (vi + 1 < 0 || vi > m->h - 1) continue;
        const uint8_t *r0 = m->a + (size_t)v0 * m->w, *r1 = m->a + (size_t)v1 * m->w;
        uint32_t wy0 = vi < 0 ? 0 : 256 - fy, wy1 = vi + 1 > m->h - 1 ? 0 : fy;
        uint32_t *row = s->px + (size_t)y * s->stride;
        for (int x = c.x0; x < c.x1; x++) {
            float u = (x + 0.5f - p->x0) * du - 0.5f;
            int ui = (int)floorf(u);
            uint32_t fx = (uint32_t)((u - ui) * 256.f);
            uint32_t wx0 = ui < 0 ? 0 : 256 - fx, wx1 = ui + 1 > m->w - 1 ? 0 : fx;
            int ua = clampi(ui, 0, m->w - 1), ub = clampi(ui + 1, 0, m->w - 1);
            uint32_t a = (r0[ua] * wx0 * wy0 + r0[ub] * wx1 * wy0 + r1[ua] * wx0 * wy1 + r1[ub] * wx1 * wy1) >> 16;
            if (a) put_cov(row + x, p->c, a > 255 ? 255 : a);
        }
    }
}

static void exec_glyphs(Surface *s, IRect c, const TxtCmd *p) {
    uint32_t col = p->c;
    for (int i = 0; i < p->n; i++) {
        const GlyphPos *g = &p->g[i];
        const Mask *m = g->m;
        if (!m || !m->a) continue;
        int x0 = MAXI(c.x0, g->x), x1 = MINI(c.x1, g->x + m->w);
        int y0 = MAXI(c.y0, g->y), y1 = MINI(c.y1, g->y + m->h);
        for (int y = y0; y < y1; y++) {
            const uint8_t *src = m->a + (size_t)(y - g->y) * m->w - g->x;
            uint32_t *row = s->px + (size_t)y * s->stride;
            for (int x = x0; x < x1; x++) {
                uint32_t a = src[x];
                if (a) put_cov(row + x, col, a);
            }
        }
    }
}

static void exec_capsule(Surface *s, IRect c, const CapCmd *p) {
    float bax = p->bx - p->ax, bay = p->by - p->ay;
    float l2 = bax * bax + bay * bay;
    float inv = l2 > 1e-6f ? 1.f / l2 : 0;
    for (int y = c.y0; y < c.y1; y++) {
        uint32_t *row = s->px + (size_t)y * s->stride;
        float py = y + 0.5f - p->ay;
        for (int x = c.x0; x < c.x1; x++) {
            float px = x + 0.5f - p->ax;
            float t = sat((px * bax + py * bay) * inv);
            float dx = px - bax * t, dy = py - bay * t;
            float d = sqrtf(dx * dx + dy * dy) - p->r;
            float cv = 0.5f - d;
            if (cv <= 0) continue;
            put_cov(row + x, p->c, cv >= 1 ? 255 : (uint32_t)(cv * 255.f));
        }
    }
}

static void exec_poly(Surface *s, IRect c, const PolyCmd *p) {
    for (int y = c.y0; y < c.y1; y++) {
        uint32_t *row = s->px + (size_t)y * s->stride;
        float py = y + 0.5f;
        for (int x = c.x0; x < c.x1; x++) {
            float px = x + 0.5f, m = 1e9f;
            for (int i = 0; i < p->n; i++) {
                float d = p->nx[i] * px + p->ny[i] * py + p->nc[i];
                if (d < m) m = d;
            }
            float cv = 0.5f + m;
            if (cv <= 0) continue;
            put_cov(row + x, p->c, cv >= 1 ? 255 : (uint32_t)(cv * 255.f));
        }
    }
}

static void exec_tile(Surface *s, IRect c, const TileCmd *p) {
    const Texture *t = p->t;
    for (int y = c.y0; y < c.y1; y++) {
        uint32_t *row = s->px + (size_t)y * s->stride;
        int ty = ((y + p->oy) % t->h + t->h) % t->h;
        const uint32_t *src = t->px + (size_t)ty * t->w;
        int tx = ((c.x0 + p->ox) % t->w + t->w) % t->w;
        for (int x = c.x0; x < c.x1; x++) {
            uint32_t px = src[tx];
            if (++tx == t->w) tx = 0;
            if (px) row[x] = blend(row[x], mul8(px, p->alpha));
        }
    }
}

/* Soft radial glow: falloff looked up by squared distance (no sqrt). */
static uint8_t glow_lut[1025];
static void glow_init(void) {
    if (glow_lut[0]) return;
    const float e = expf(-4.5f);
    for (int i = 0; i <= 1024; i++) {
        float t2 = i / 1024.f;
        float v = (expf(-4.5f * t2) - e) / (1 - e);
        glow_lut[i] = (uint8_t)(v * 255.f + 0.5f);
    }
}
static void exec_glow(Surface *s, IRect c, const GlowCmd *p) {
    float inv = 1024.f / (p->r * p->r);
    for (int y = c.y0; y < c.y1; y++) {
        float dy = y + 0.5f - p->cy;
        float rem = p->r * p->r - dy * dy;
        if (rem <= 0) continue;
        float half = sqrtf(rem);
        int x0 = MAXI(c.x0, (int)(p->cx - half)), x1 = MINI(c.x1, (int)(p->cx + half) + 1);
        uint32_t *row = s->px + (size_t)y * s->stride;
        float dy2 = dy * dy;
        float dx = x0 + 0.5f - p->cx;
        for (int x = x0; x < x1; x++, dx += 1.f) {
            int i = (int)((dx * dx + dy2) * inv);
            if (i >= 1024) continue;
            uint32_t a = glow_lut[i];
            if (a) row[x] = blend(row[x], mul8(p->c, a));
        }
    }
}

static void exec_cmd(Surface *s, IRect c, const Cmd *cmd) {
    switch (cmd->type) {
    case C_RRECT: exec_rrect(s, c, &cmd->rr); break;
    case C_IMAGE: exec_image(s, c, &cmd->img); break;
    case C_MASK: exec_mask(s, c, &cmd->msk); break;
    case C_GLYPHS: exec_glyphs(s, c, &cmd->txt); break;
    case C_CAPSULE: exec_capsule(s, c, &cmd->cap); break;
    case C_POLY: exec_poly(s, c, &cmd->poly); break;
    case C_TILE: exec_tile(s, c, &cmd->tile); break;
    case C_GLOW: exec_glow(s, c, &cmd->glow); break;
    }
}

/* ======================= Backdrop blur ======================= */
static void slot_downsample_row(Slot *sl, const Surface *fb, int j) {
    const int x0 = sl->src.x0, y0 = sl->src.y0 + 4 * j;
    uint32_t *out = sl->buf + (size_t)j * sl->lw;
    for (int i = 0; i < sl->lw; i++) {
        uint32_t r = 0, g = 0, b = 0;
        for (int yy = 0; yy < 4; yy++) {
            const uint32_t *p = fb->px + (size_t)(y0 + yy) * fb->stride + x0 + 4 * i;
            for (int xx = 0; xx < 4; xx++) {
                uint32_t c = p[xx];
                r += (c >> 16) & 255; g += (c >> 8) & 255; b += c & 255;
            }
        }
        out[i] = 0xFF000000u | ((r >> 4) << 16) | ((g >> 4) << 8) | (b >> 4);
    }
}
static void blur_run(uint32_t *p, int n, int stride, int r, int *tmp, int finish, uint32_t tint, float satur) {
    int *ch = tmp, *out = tmp + 3 * n;
    for (int i = 0; i < n; i++) {
        uint32_t c = p[(size_t)i * stride];
        ch[i] = (c >> 16) & 255; ch[n + i] = (c >> 8) & 255; ch[2 * n + i] = c & 255;
    }
    int div = 2 * r + 1, inv = (1 << 16) / div;
    for (int pass = 0; pass < 3; pass++) {
        for (int k = 0; k < 3; k++) {
            int *in = ch + k * n, *o = out + k * n;
            int s = in[0] * (r + 1);
            for (int i = 1; i <= r; i++) s += in[MINI(i, n - 1)];
            for (int i = 0; i < n; i++) {
                o[i] = (s * inv) >> 16;
                s += in[MINI(i + r + 1, n - 1)] - in[MAXI(i - r, 0)];
            }
        }
        int *sw = ch; ch = out; out = sw;
    }
    int ta = tint >> 24, tr = (tint >> 16) & 255, tg = (tint >> 8) & 255, tb = tint & 255;
    int s256 = (int)(satur * 256.f);
    for (int i = 0; i < n; i++) {
        int r0 = ch[i], g0 = ch[n + i], b0 = ch[2 * n + i];
        if (finish) {
            int l = (r0 * 54 + g0 * 183 + b0 * 19) >> 8;
            r0 = l + (((r0 - l) * s256) >> 8);
            g0 = l + (((g0 - l) * s256) >> 8);
            b0 = l + (((b0 - l) * s256) >> 8);
            r0 = clampi(r0, 0, 255); g0 = clampi(g0, 0, 255); b0 = clampi(b0, 0, 255);
            r0 = ((r0 * (255 - ta)) >> 8) + tr;
            g0 = ((g0 * (255 - ta)) >> 8) + tg;
            b0 = ((b0 * (255 - ta)) >> 8) + tb;
            r0 = MINI(r0, 255); g0 = MINI(g0, 255); b0 = MINI(b0, 255);
        }
        p[(size_t)i * stride] = 0xFF000000u | ((uint32_t)r0 << 16) | ((uint32_t)g0 << 8) | (uint32_t)b0;
    }
}

typedef struct { Gfx *g; Surface *fb; int s0, s1; int stage; int *prefix; int **tmp; } BlurJob;

static void blur_item(void *ctx, int idx, int thread) {
    BlurJob *bj = ctx;
    int k = bj->s0;
    while (k + 1 < bj->s1 && bj->prefix[k + 1 - bj->s0] <= idx) k++;
    Slot *sl = bj->g->slots[k];
    int local = idx - bj->prefix[k - bj->s0];
    int *tmp = bj->tmp[thread];
    if (sl->capture) {
        memcpy(sl->buf + (size_t)local * sl->lw, bj->fb->px + (size_t)(sl->src.y0 + local) * bj->fb->stride + sl->src.x0, (size_t)sl->lw * 4);
        return;
    }
    if (bj->stage == 0) slot_downsample_row(sl, bj->fb, local);
    else if (bj->stage == 1) blur_run(sl->buf + (size_t)local * sl->lw, sl->lw, 1, sl->radius, tmp, 0, 0, 1.f);
    else blur_run(sl->buf + local, sl->lh, sl->lw, sl->radius, tmp, 1, sl->tint, sl->sat);
}

static int *g_blur_tmp[64];

static void run_blur_group(Gfx *g, Surface *fb, int c0, int c1) {
    int s0 = g->cmds[c0].blur.slot, s1 = g->cmds[c1 - 1].blur.slot + 1;
    int ns = s1 - s0;
    int prefix[64 + 1];
    if (ns > 64) ns = 64, s1 = s0 + 64;
    int nt = jobs_render_threads();
    for (int i = 0; i < nt && i < 64; i++)
        if (!g_blur_tmp[i]) g_blur_tmp[i] = xmalloc(sizeof(int) * 6 * 4096);
    BlurJob bj = {g, fb, s0, s1, 0, prefix, g_blur_tmp};
    for (int stage = 0; stage < 3; stage++) {
        prefix[0] = 0;
        for (int k = s0; k < s1; k++) {
            Slot *sl = g->slots[k];
            int cnt = stage == 2 ? sl->lw : sl->lh;
            if (sl->capture && stage > 0) cnt = 0;
            prefix[k - s0 + 1] = prefix[k - s0] + cnt;
        }
        bj.stage = stage;
        parallel_for(prefix[ns], blur_item, &bj);
    }
    for (int k = s0; k < s1; k++) g->slots[k]->tex.ready = 1;
}

/* ======================= Execution ======================= */
#define BAND_H 30
typedef struct { Gfx *g; Surface *fb; int c0, c1; } PhaseJob;

static void phase_band(void *ctx, int band, int thread) {
    (void)thread;
    PhaseJob *pj = ctx;
    IRect b = {0, band * BAND_H, pj->fb->w, MINI(pj->fb->h, (band + 1) * BAND_H)};
    for (int i = pj->c0; i < pj->c1; i++) {
        const Cmd *cmd = &pj->g->cmds[i];
        IRect c = {MAXI(b.x0, cmd->clip.x0), MAXI(b.y0, cmd->clip.y0), MINI(b.x1, cmd->clip.x1), MINI(b.y1, cmd->clip.y1)};
        if (c.x0 >= c.x1 || c.y0 >= c.y1) continue;
        exec_cmd(pj->fb, c, cmd);
    }
}

int g_profile;
double g_prof[10];
static const char *PROF_NAMES[10] = {"?", "rrect", "image", "mask", "glyphs", "capsule", "poly", "tile", "blur", "glow"};
void gfx_profile_dump(void) {
    double tot = 0;
    for (int i = 0; i < 10; i++) tot += g_prof[i];
    for (int i = 0; i < 10; i++)
        if (g_prof[i] > 0) fprintf(stderr, "    %-8s %7.2f ms  %4.1f%%\n", PROF_NAMES[i], g_prof[i] * 1000, 100 * g_prof[i] / tot);
    memset(g_prof, 0, sizeof g_prof);
}

void gfx_execute(Gfx *g, Surface *fb) {
    int n = g->ncmds, i = 0;
    if (g_profile) {
        IRect full = {0, 0, fb->w, fb->h};
        for (int k = 0; k < n; k++) {
            Cmd *c = &g->cmds[k];
            double t0 = now_sec();
            if (c->type == C_BLUR) { run_blur_group(g, fb, k, k + 1); }
            else {
                IRect r = {MAXI(full.x0, c->clip.x0), MAXI(full.y0, c->clip.y0), MINI(full.x1, c->clip.x1), MINI(full.y1, c->clip.y1)};
                if (r.x0 < r.x1 && r.y0 < r.y1) exec_cmd(fb, r, c);
            }
            g_prof[c->type] += now_sec() - t0;
        }
        return;
    }
    int nbands = (fb->h + BAND_H - 1) / BAND_H;
    g_stat_blur_regions = g->nslots;
    while (i < n) {
        int j = i;
        while (j < n && g->cmds[j].type != C_BLUR) j++;
        if (j > i) {
            PhaseJob pj = {g, fb, i, j};
            parallel_for(nbands, phase_band, &pj);
        }
        int k = j;
        while (k < n && g->cmds[k].type == C_BLUR) k++;
        if (k > j) run_blur_group(g, fb, j, k);
        i = k;
    }
}

/* ======================= Recording ======================= */
Gfx *gfx_new_recorder(void) {
    Gfx *g = xcalloc(1, sizeof *g);
    g->capcmds = 4096;
    g->cmds = xmalloc(sizeof(Cmd) * g->capcmds);
    g->st[0].alpha = 1.f;
    g->st[0].clip = (IRect){0, 0, SCREEN_W, SCREEN_H};
    return g;
}
Gfx *gfx_new_immediate(Surface target) {
    Gfx *g = gfx_new_recorder();
    g->immediate = 1;
    gfx_set_target(g, target);
    return g;
}
void gfx_set_target(Gfx *g, Surface target) {
    g->target = target;
    g->sp = 0;
    g->st[0].tx = g->st[0].ty = 0;
    g->st[0].alpha = 1.f;
    g->st[0].clip = (IRect){0, 0, target.w, target.h};
}
void gfx_free(Gfx *g) {
    Chunk *c = g->chunks;
    while (c) { Chunk *n = c->next; free(c); c = n; }
    for (int i = 0; i < g->capslots; i++) if (g->slots[i]) { free(g->slots[i]->buf); free(g->slots[i]); }
    free(g->slots);
    free(g->cmds);
    free(g);
}
void gfx_begin(Gfx *g) {
    g->ncmds = 0;
    g->nslots = 0;
    g->sp = 0;
    g->st[0].tx = g->st[0].ty = 0;
    g->st[0].alpha = 1.f;
    g->st[0].clip = (IRect){0, 0, SCREEN_W, SCREEN_H};
    for (Chunk *c = g->chunks; c; c = c->next) c->used = 0;
    g->cur = g->chunks;
}
int gfx_cmd_count(Gfx *g) { return g->ncmds; }

void *gfx_alloc(Gfx *g, size_t n) {
    n = (n + 15) & ~(size_t)15;
    while (g->cur && g->cur->used + n > g->cur->cap) g->cur = g->cur->next;
    if (!g->cur) {
        size_t cap = n > (1 << 20) ? n : (1 << 20);
        Chunk *c = xmalloc(sizeof(Chunk) + cap);
        c->cap = cap; c->used = 0; c->next = g->chunks;
        g->chunks = c;
        g->cur = c;
    }
    void *p = g->cur->data + g->cur->used;
    g->cur->used += n;
    return p;
}

void g_push(Gfx *g) {
    if (g->sp < 47) { g->st[g->sp + 1] = g->st[g->sp]; g->sp++; }
}
void g_pop(Gfx *g) { if (g->sp > 0) g->sp--; }
void g_translate(Gfx *g, float dx, float dy) { g->st[g->sp].tx += dx; g->st[g->sp].ty += dy; }
void g_opacity(Gfx *g, float a) { g->st[g->sp].alpha *= sat(a); }
float g_get_opacity(Gfx *g) { return g->st[g->sp].alpha; }
void g_clip(Gfx *g, float x, float y, float w, float h) {
    GState *s = &g->st[g->sp];
    IRect c = {(int)floorf(x + s->tx), (int)floorf(y + s->ty), (int)ceilf(x + w + s->tx), (int)ceilf(y + h + s->ty)};
    s->clip.x0 = MAXI(s->clip.x0, c.x0); s->clip.y0 = MAXI(s->clip.y0, c.y0);
    s->clip.x1 = MINI(s->clip.x1, c.x1); s->clip.y1 = MINI(s->clip.y1, c.y1);
}

static Cmd *emit(Gfx *g, int type, float bx0, float by0, float bx1, float by1) {
    GState *s = &g->st[g->sp];
    IRect c = {MAXI(s->clip.x0, (int)floorf(bx0)), MAXI(s->clip.y0, (int)floorf(by0)),
               MINI(s->clip.x1, (int)ceilf(bx1)), MINI(s->clip.y1, (int)ceilf(by1))};
    if (g->immediate) {
        c.x0 = MAXI(c.x0, 0); c.y0 = MAXI(c.y0, 0);
        c.x1 = MINI(c.x1, g->target.w); c.y1 = MINI(c.y1, g->target.h);
    }
    if (c.x0 >= c.x1 || c.y0 >= c.y1) return NULL;
    static __thread Cmd scratch;
    Cmd *cmd;
    if (g->immediate) cmd = &scratch;
    else {
        if (g->ncmds == g->capcmds) {
            g->capcmds *= 2;
            g->cmds = realloc(g->cmds, sizeof(Cmd) * g->capcmds);
            if (!g->cmds) abort();
        }
        cmd = &g->cmds[g->ncmds++];
    }
    cmd->type = type;
    cmd->clip = c;
    return cmd;
}
static void done(Gfx *g, Cmd *cmd) {
    if (g->immediate && cmd) exec_cmd(&g->target, cmd->clip, cmd);
}

static void rrect_full(Gfx *g, float x, float y, float w, float h, float r, float f, float stroke,
                       uint32_t c0, uint32_t c1, float gx0, float gy0, float gx1, float gy1) {
    GState *s = &g->st[g->sp];
    if (w <= 0 || h <= 0) return;
    c0 = col_alpha(c0, s->alpha); c1 = col_alpha(c1, s->alpha);
    if (!c0 && !c1) return;
    x += s->tx; y += s->ty;
    float pad = f + 1;
    Cmd *cmd = emit(g, C_RRECT, x - pad, y - pad, x + w + pad, y + h + pad);
    if (!cmd) return;
    RRCmd *p = &cmd->rr;
    p->x0 = x; p->y0 = y; p->x1 = x + w; p->y1 = y + h;
    p->r = r; p->f = f; p->stroke = stroke; p->c0 = c0; p->c1 = c1;
    float dx = gx1 - gx0, dy = gy1 - gy0, l2 = dx * dx + dy * dy;
    p->gx = gx0 + s->tx; p->gy = gy0 + s->ty;
    p->gdx = l2 > 0 ? dx / l2 : 0; p->gdy = l2 > 0 ? dy / l2 : 0;
    done(g, cmd);
}
void g_rect(Gfx *g, float x, float y, float w, float h, float r, uint32_t c) {
    rrect_full(g, x, y, w, h, r, 1, 0, c, c, 0, 0, 0, 0);
}
void g_rect_vgrad(Gfx *g, float x, float y, float w, float h, float r, uint32_t top, uint32_t bottom) {
    rrect_full(g, x, y, w, h, r, 1, 0, top, bottom, x, y, x, y + h);
}
void g_rect_hgrad(Gfx *g, float x, float y, float w, float h, float r, uint32_t left, uint32_t right) {
    rrect_full(g, x, y, w, h, r, 1, 0, left, right, x, y, x + w, y);
}
void g_rect_grad(Gfx *g, float x, float y, float w, float h, float r, uint32_t c0, uint32_t c1,
                 float gx0, float gy0, float gx1, float gy1) {
    rrect_full(g, x, y, w, h, r, 1, 0, c0, c1, gx0, gy0, gx1, gy1);
}
void g_shadow(Gfx *g, float x, float y, float w, float h, float r, float blur, uint32_t c) {
    rrect_full(g, x, y, w, h, r + blur * 0.5f, blur, 0, c, c, 0, 0, 0, 0);
}
void g_stroke(Gfx *g, float x, float y, float w, float h, float r, float width, uint32_t c) {
    rrect_full(g, x, y, w, h, r, 1, width, c, c, 0, 0, 0, 0);
}
void g_circle(Gfx *g, float cx, float cy, float rad, uint32_t c) {
    rrect_full(g, cx - rad, cy - rad, rad * 2, rad * 2, rad, 1, 0, c, c, 0, 0, 0, 0);
}
void g_glow(Gfx *g, float cx, float cy, float rad, uint32_t c) {
    GState *s = &g->st[g->sp];
    c = col_alpha(c, s->alpha);
    if (!c || rad <= 0.5f) return;
    glow_init();
    cx += s->tx; cy += s->ty;
    float R = rad * 1.25f;
    Cmd *cmd = emit(g, C_GLOW, cx - R, cy - R, cx + R, cy + R);
    if (!cmd) return;
    cmd->glow = (GlowCmd){cx, cy, R, c};
    done(g, cmd);
}
void g_line(Gfx *g, float x0, float y0, float x1, float y1, float width, uint32_t c) {
    GState *s = &g->st[g->sp];
    c = col_alpha(c, s->alpha);
    if (!c) return;
    x0 += s->tx; x1 += s->tx; y0 += s->ty; y1 += s->ty;
    float r = width * 0.5f;
    Cmd *cmd = emit(g, C_CAPSULE, fminf(x0, x1) - r - 1, fminf(y0, y1) - r - 1, fmaxf(x0, x1) + r + 1, fmaxf(y0, y1) + r + 1);
    if (!cmd) return;
    cmd->cap = (CapCmd){x0, y0, x1, y1, r, c};
    done(g, cmd);
}
void g_poly(Gfx *g, const float *xy, int n, uint32_t c) {
    GState *s = &g->st[g->sp];
    c = col_alpha(c, s->alpha);
    if (!c || n < 3) return;
    if (n > 8) n = 8;
    float px[8], py[8], bx0 = 1e9f, by0 = 1e9f, bx1 = -1e9f, by1 = -1e9f, area = 0;
    for (int i = 0; i < n; i++) {
        px[i] = xy[2 * i] + s->tx; py[i] = xy[2 * i + 1] + s->ty;
        bx0 = fminf(bx0, px[i]); by0 = fminf(by0, py[i]); bx1 = fmaxf(bx1, px[i]); by1 = fmaxf(by1, py[i]);
    }
    for (int i = 0; i < n; i++) { int j = (i + 1) % n; area += px[i] * py[j] - px[j] * py[i]; }
    Cmd *cmd = emit(g, C_POLY, bx0 - 1, by0 - 1, bx1 + 1, by1 + 1);
    if (!cmd) return;
    PolyCmd *p = &cmd->poly;
    p->n = n; p->c = c;
    float sgn = area > 0 ? 1.f : -1.f;
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        float ex = px[j] - px[i], ey = py[j] - py[i], l = sqrtf(ex * ex + ey * ey);
        if (l < 1e-6f) l = 1e-6f;
        /* inward normal for CCW (screen coords, area>0 means clockwise visually) */
        float nx = -ey / l * sgn, ny = ex / l * sgn;
        p->nx[i] = nx; p->ny[i] = ny; p->nc[i] = -(nx * px[i] + ny * py[i]);
    }
    done(g, cmd);
}
void g_image_src(Gfx *g, const Texture *t, float sx, float sy, float sw, float sh,
                 float x, float y, float w, float h, float r, float alpha) {
    GState *s = &g->st[g->sp];
    if (!t || !t->px || w <= 0 || h <= 0 || sw <= 0 || sh <= 0) return;
    alpha *= s->alpha;
    if (alpha <= 0.002f) return;
    /* Pick the mip level whose scale is closest above 1:2. */
    while (t->mip && t->mip->ready && w < sw * 0.55f && h < sh * 0.55f) {
        float kx = (float)t->mip->w / t->w, ky = (float)t->mip->h / t->h;
        sx *= kx; sw *= kx; sy *= ky; sh *= ky;
        t = t->mip;
    }
    x += s->tx; y += s->ty;
    Cmd *cmd = emit(g, C_IMAGE, x - 1, y - 1, x + w + 1, y + h + 1);
    if (!cmd) return;
    ImgCmd *p = &cmd->img;
    p->t = t; p->x0 = x; p->y0 = y; p->x1 = x + w; p->y1 = y + h;
    p->sx0 = sx; p->sy0 = sy; p->sx1 = sx + sw; p->sy1 = sy + sh;
    p->r = r; p->alpha = (uint32_t)(sat(alpha) * 255.f + 0.5f);
    done(g, cmd);
}
void g_image(Gfx *g, const Texture *t, float x, float y, float w, float h, float r, float alpha) {
    if (!t) return;
    g_image_src(g, t, 0, 0, (float)t->w, (float)t->h, x, y, w, h, r, alpha);
}
void g_image_cover(Gfx *g, const Texture *t, float x, float y, float w, float h, float r, float alpha) {
    if (!t) return;
    float ta = (float)t->w / t->h, da = w / h;
    float sw = (float)t->w, sh = (float)t->h, sx = 0, sy = 0;
    if (ta > da) { sw = t->h * da; sx = (t->w - sw) * 0.5f; }
    else { sh = t->w / da; sy = (t->h - sh) * 0.5f; }
    g_image_src(g, t, sx, sy, sw, sh, x, y, w, h, r, alpha);
}
void g_tile(Gfx *g, const Texture *t, float x, float y, float w, float h, float ox, float oy, float alpha) {
    GState *s = &g->st[g->sp];
    alpha *= s->alpha;
    if (!t || alpha <= 0.002f) return;
    x += s->tx; y += s->ty;
    Cmd *cmd = emit(g, C_TILE, x, y, x + w, y + h);
    if (!cmd) return;
    cmd->tile = (TileCmd){t, (int)ox, (int)oy, (uint32_t)(sat(alpha) * 255.f)};
    done(g, cmd);
}
void g_mask(Gfx *g, const Mask *m, float x, float y, float w, float h, uint32_t c) {
    GState *s = &g->st[g->sp];
    c = col_alpha(c, s->alpha);
    if (!m || !c || w <= 0 || h <= 0) return;
    x += s->tx; y += s->ty;
    Cmd *cmd = emit(g, C_MASK, x - 1, y - 1, x + w + 1, y + h + 1);
    if (!cmd) return;
    cmd->msk = (MaskCmd){m, x, y, x + w, y + h, c};
    done(g, cmd);
}
void g_glyphs(Gfx *g, const GlyphPos *gp, int n, uint32_t c) {
    GState *s = &g->st[g->sp];
    c = col_alpha(c, s->alpha);
    if (!c || n <= 0) return;
    int tx = (int)roundf(s->tx), ty = (int)roundf(s->ty);
    float bx0 = 1e9f, by0 = 1e9f, bx1 = -1e9f, by1 = -1e9f;
    GlyphPos *copy = g->immediate ? xmalloc(sizeof(GlyphPos) * n) : gfx_alloc(g, sizeof(GlyphPos) * n);
    int k = 0;
    for (int i = 0; i < n; i++) {
        if (!gp[i].m || !gp[i].m->a) continue;
        copy[k] = gp[i];
        copy[k].x += tx; copy[k].y += ty;
        bx0 = fminf(bx0, copy[k].x); by0 = fminf(by0, copy[k].y);
        bx1 = fmaxf(bx1, copy[k].x + gp[i].m->w); by1 = fmaxf(by1, copy[k].y + gp[i].m->h);
        k++;
    }
    if (k) {
        Cmd *cmd = emit(g, C_GLYPHS, bx0, by0, bx1, by1);
        if (cmd) {
            cmd->txt = (TxtCmd){copy, k, c};
            done(g, cmd);
        }
    }
    if (g->immediate) free(copy);
}

void g_backdrop(Gfx *g, float x, float y, float w, float h, float r, int radius,
                uint32_t tint, float saturation) {
    GState *s = &g->st[g->sp];
    if (w <= 0 || h <= 0) return;
    radius = clampi(radius, 1, 24);
    float a = s->alpha;
    x += s->tx; y += s->ty;
    Surface *tgt = &g->target;
    int fw = g->immediate ? tgt->w : SCREEN_W, fh = g->immediate ? tgt->h : SCREEN_H;
    int m = radius * 4 * 2;
    IRect src = {((int)floorf(x) - m) & ~3, ((int)floorf(y) - m) & ~3, ((int)ceilf(x + w) + m + 3) & ~3, ((int)ceilf(y + h) + m + 3) & ~3};
    src.x0 = MAXI(src.x0, 0); src.y0 = MAXI(src.y0, 0);
    src.x1 = MINI(src.x1, fw & ~3); src.y1 = MINI(src.y1, fh & ~3);
    /* Don't capture beyond the clip: the glass only ever shows the visible part. */
    if (src.x1 - src.x0 < 8 || src.y1 - src.y0 < 8) return;
    if (g->nslots == g->capslots) {
        int nc = g->capslots ? g->capslots * 2 : 16;
        g->slots = realloc(g->slots, sizeof(Slot *) * nc);
        for (int i = g->capslots; i < nc; i++) g->slots[i] = xcalloc(1, sizeof(Slot));
        g->capslots = nc;
    }
    int si = g->nslots;
    Slot *sl = g->slots[si];
    sl->src = src;
    sl->capture = 0;
    sl->lw = (src.x1 - src.x0) / 4;
    sl->lh = (src.y1 - src.y0) / 4;
    sl->radius = radius;
    sl->tint = tint;
    sl->sat = saturation;
    size_t need = (size_t)sl->lw * sl->lh;
    if (need > sl->cap) { free(sl->buf); sl->buf = xmalloc(need * 4); sl->cap = need; }
    sl->tex.w = sl->lw; sl->tex.h = sl->lh; sl->tex.px = sl->buf; sl->tex.opaque = 1; sl->tex.mip = NULL;
    sl->tex.ready = 0;
    float sx = (x - src.x0) / 4.f, sy = (y - src.y0) / 4.f;
    if (g->immediate) {
        int *tmp = xmalloc(sizeof(int) * 6 * 4096);
        for (int j = 0; j < sl->lh; j++) slot_downsample_row(sl, tgt, j);
        for (int j = 0; j < sl->lh; j++) blur_run(sl->buf + (size_t)j * sl->lw, sl->lw, 1, radius, tmp, 0, 0, 1);
        for (int i = 0; i < sl->lw; i++) blur_run(sl->buf + i, sl->lh, sl->lw, radius, tmp, 1, tint, saturation);
        free(tmp);
        sl->tex.ready = 1;
    } else {
        g->nslots++;
        Cmd *cmd = emit(g, C_BLUR, 0, 0, fw, fh);
        if (!cmd) { g->nslots--; return; }
        cmd->clip = (IRect){0, 0, fw, fh};
        cmd->blur.slot = si;
    }
    /* Composite the frosted region back under the glass shape. */
    float px = x - s->tx, py = y - s->ty;
    g_image_src(g, &sl->tex, sx, sy, w / 4.f, h / 4.f, px, py, w, h, r, a);
}

/* ---- Layers: content composited back with a rounded, anti-aliased mask ---- */
static Slot *new_slot(Gfx *g, int *index) {
    if (g->nslots == g->capslots) {
        int nc = g->capslots ? g->capslots * 2 : 16;
        g->slots = realloc(g->slots, sizeof(Slot *) * nc);
        for (int i = g->capslots; i < nc; i++) g->slots[i] = xcalloc(1, sizeof(Slot));
        g->capslots = nc;
    }
    *index = g->nslots;
    return g->slots[g->nslots];
}

static int capture(Gfx *g, IRect r) {
    int si;
    Slot *sl = new_slot(g, &si);
    sl->src = r;
    sl->capture = 1;
    sl->lw = r.x1 - r.x0;
    sl->lh = r.y1 - r.y0;
    size_t need = (size_t)sl->lw * sl->lh;
    if (need > sl->cap) { free(sl->buf); sl->buf = xmalloc(need * 4); sl->cap = need; }
    sl->tex = (Texture){sl->lw, sl->lh, sl->buf, NULL, 1, 0, 0};
    if (g->immediate) {
        for (int y = 0; y < sl->lh; y++)
            memcpy(sl->buf + (size_t)y * sl->lw, g->target.px + (size_t)(r.y0 + y) * g->target.stride + r.x0, (size_t)sl->lw * 4);
        sl->tex.ready = 1;
        return si;
    }
    g->nslots++;
    if (g->ncmds == g->capcmds) {
        g->capcmds *= 2;
        g->cmds = realloc(g->cmds, sizeof(Cmd) * g->capcmds);
        if (!g->cmds) abort();
    }
    Cmd *cmd = &g->cmds[g->ncmds++];
    cmd->type = C_BLUR;
    cmd->clip = (IRect){0, 0, SCREEN_W, SCREEN_H};
    cmd->blur.slot = si;
    return si;
}

static IRect layer_rect(Gfx *g, float x, float y, float w, float h) {
    GState *s = &g->st[g->sp];
    int fw = g->immediate ? g->target.w : SCREEN_W, fh = g->immediate ? g->target.h : SCREEN_H;
    IRect r = {(int)floorf(x + s->tx), (int)floorf(y + s->ty), (int)ceilf(x + w + s->tx), (int)ceilf(y + h + s->ty)};
    r.x0 = MAXI(r.x0, 0); r.y0 = MAXI(r.y0, 0); r.x1 = MINI(r.x1, fw); r.y1 = MINI(r.y1, fh);
    return r;
}

static int layer_bg[8], layer_sp;
static IRect layer_r[8];

void g_layer_begin(Gfx *g, float x, float y, float w, float h) {
    IRect r = layer_rect(g, x, y, w, h);
    if (layer_sp >= 8) return;
    layer_r[layer_sp] = r;
    layer_bg[layer_sp] = (r.x1 > r.x0 && r.y1 > r.y0) ? capture(g, r) : -1;
    layer_sp++;
}

void g_layer_end(Gfx *g, float radius, float alpha) {
    if (layer_sp <= 0) return;
    layer_sp--;
    IRect r = layer_r[layer_sp];
    int bg = layer_bg[layer_sp];
    if (bg < 0) return;
    int fg = capture(g, r);
    Slot *b = g->slots[bg], *f = g->slots[fg];
    float w = (float)(r.x1 - r.x0), h = (float)(r.y1 - r.y0);
    g_push(g);
    g->st[g->sp].tx = g->st[g->sp].ty = 0;
    g->st[g->sp].alpha = 1;
    g_image(g, &b->tex, (float)r.x0, (float)r.y0, w, h, 0, 1);
    g->st[g->sp].alpha = alpha;
    g_image(g, &f->tex, (float)r.x0, (float)r.y0, w, h, radius, 1);
    g_pop(g);
}

/* Content hash of the recorded frame: identical lists produce identical
 * pixels, so the caller can skip rendering and presenting entirely. */
uint64_t gfx_hash(Gfx *g) {
    uint64_t h = 1469598103934665603ull;
#define MIX(v) do { h ^= (uint64_t)(v); h *= 1099511628211ull; } while (0)
    for (int i = 0; i < g->ncmds; i++) {
        const Cmd *c = &g->cmds[i];
        const uint32_t *w = (const uint32_t *)c;
        for (size_t k = 0; k < sizeof(Cmd) / 4; k++) MIX(w[k]);
        if (c->type == C_GLYPHS)
            for (int k = 0; k < c->txt.n; k++) { MIX((uintptr_t)c->txt.g[k].m); MIX(c->txt.g[k].x); MIX(c->txt.g[k].y); }
        if (c->type == C_IMAGE) { MIX(c->img.t->ready); MIX(c->img.t->w); }
    }
    MIX(g->ncmds);
#undef MIX
    return h;
}
