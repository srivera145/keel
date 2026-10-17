/* KEEL OS — font engine.
 * Parses sfnt containers with either TrueType (glyf/loca, quadratic) or CFF
 * (Type 2 charstrings, cubic) outlines, rasterizes glyphs with the shared
 * coverage rasterizer and caches them per pixel size. Uses the system's
 * fonts at runtime (Inter preferred); nothing is bundled. */
#define _DEFAULT_SOURCE
#include "font.h"
#include "raster.h"
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>

typedef struct { const uint8_t *p; int count, offsize; const uint8_t *data; } CffIndex;

typedef struct {
    uint8_t *data;
    size_t size;
    int upem, nglyphs, loc_long, nhmetrics;
    uint32_t loca, glyf, hmtx, cmap;
    int cmap_fmt;
    int ascender, descender;
    float cap; /* cap height in font units */
    int is_cff;
    CffIndex charstrings, gsubrs, subrs;
    int gbias, bias;
    uint16_t ascii_gid[128];
} FontFile;

static FontFile g_fonts[FONT_COUNT];
static char g_font_paths[FONT_COUNT][512];
static pthread_mutex_t g_glyph_lock = PTHREAD_MUTEX_INITIALIZER;

static inline uint32_t rd8(const uint8_t *p) { return p[0]; }
static inline uint32_t rd16(const uint8_t *p) { return (uint32_t)p[0] << 8 | p[1]; }
static inline int32_t rds16(const uint8_t *p) { return (int16_t)rd16(p); }
static inline uint32_t rd32(const uint8_t *p) { return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3]; }

static uint32_t find_table(const uint8_t *d, const char *tag) {
    int n = (int)rd16(d + 4);
    for (int i = 0; i < n; i++) {
        const uint8_t *r = d + 12 + 16 * i;
        if (memcmp(r, tag, 4) == 0) return rd32(r + 8);
    }
    return 0;
}

/* ---------------- CFF ---------------- */
static uint32_t rd_off(const uint8_t *p, int sz) {
    uint32_t v = 0;
    for (int i = 0; i < sz; i++) v = v << 8 | p[i];
    return v;
}
static const uint8_t *cff_index(const uint8_t *p, CffIndex *ix) {
    ix->count = (int)rd16(p);
    if (ix->count == 0) { ix->offsize = 0; ix->p = p; ix->data = p + 2; return p + 2; }
    ix->offsize = p[2];
    ix->p = p + 3;
    ix->data = p + 3 + (ix->count + 1) * ix->offsize - 1;
    uint32_t last = rd_off(ix->p + ix->count * ix->offsize, ix->offsize);
    return ix->data + last;
}
static const uint8_t *cff_get(const CffIndex *ix, int i, int *len) {
    if (i < 0 || i >= ix->count) { *len = 0; return NULL; }
    uint32_t a = rd_off(ix->p + i * ix->offsize, ix->offsize);
    uint32_t b = rd_off(ix->p + (i + 1) * ix->offsize, ix->offsize);
    *len = (int)(b - a);
    return ix->data + a;
}
static int cff_bias(int n) { return n < 1240 ? 107 : (n < 33900 ? 1131 : 32768); }

/* Parse a DICT, return operand(s) for an operator (op>=1200 for escaped). */
static int dict_get(const uint8_t *p, int len, int op, float *out, int maxn) {
    float st[48];
    int sp = 0;
    const uint8_t *e = p + len;
    while (p < e) {
        int b = *p;
        if (b <= 21) {
            int o = b;
            p++;
            if (b == 12) { o = 1200 + *p; p++; }
            if (o == op) {
                int n = sp < maxn ? sp : maxn;
                for (int i = 0; i < n; i++) out[i] = st[i];
                return n;
            }
            sp = 0;
        } else {
            float v;
            if (b == 28) { v = (float)(int16_t)rd16(p + 1); p += 3; }
            else if (b == 29) { v = (float)(int32_t)rd32(p + 1); p += 5; }
            else if (b == 30) { p++; while (p < e) { int c = *p++; if ((c & 15) == 15 || (c >> 4) == 15) break; } v = 0; }
            else if (b >= 32 && b <= 246) { v = (float)(b - 139); p++; }
            else if (b >= 247 && b <= 250) { v = (float)((b - 247) * 256 + p[1] + 108); p += 2; }
            else if (b >= 251 && b <= 254) { v = (float)(-(b - 251) * 256 - p[1] - 108); p += 2; }
            else { p++; continue; }
            if (sp < 48) st[sp++] = v;
        }
    }
    return 0;
}

static int cff_init(FontFile *f, uint32_t off) {
    const uint8_t *base = f->data + off;
    const uint8_t *p = base + base[2];
    CffIndex names, topd, strings;
    p = cff_index(p, &names);
    p = cff_index(p, &topd);
    p = cff_index(p, &strings);
    p = cff_index(p, &f->gsubrs);
    int tlen;
    const uint8_t *top = cff_get(&topd, 0, &tlen);
    if (!top) return 0;
    float v[4];
    if (dict_get(top, tlen, 17, v, 1) != 1) return 0;
    cff_index(base + (int)v[0], &f->charstrings);
    f->subrs.count = 0;
    if (dict_get(top, tlen, 18, v, 2) == 2) {
        const uint8_t *priv = base + (int)v[1];
        float s[1];
        if (dict_get(priv, (int)v[0], 19, s, 1) == 1) cff_index(priv + (int)s[0], &f->subrs);
    }
    f->gbias = cff_bias(f->gsubrs.count);
    f->bias = cff_bias(f->subrs.count);
    f->nglyphs = f->charstrings.count;
    return 1;
}

typedef struct { Path *path; float x, y; int started; int nstems; float tx[6]; } CsCtx;

static void cs_pt(CsCtx *c, float x, float y, float *ox, float *oy) {
    *ox = c->tx[0] * x + c->tx[1] * y + c->tx[2];
    *oy = c->tx[3] * x + c->tx[4] * y + c->tx[5];
}
static void cs_move(CsCtx *c, float dx, float dy) {
    c->x += dx; c->y += dy;
    float px, py;
    cs_pt(c, c->x, c->y, &px, &py);
    path_move(c->path, px, py);
    c->started = 1;
}
static void cs_line(CsCtx *c, float dx, float dy) {
    if (!c->started) cs_move(c, 0, 0);
    c->x += dx; c->y += dy;
    float px, py;
    cs_pt(c, c->x, c->y, &px, &py);
    path_line(c->path, px, py);
}
static void cs_curve(CsCtx *c, float dx1, float dy1, float dx2, float dy2, float dx3, float dy3) {
    if (!c->started) cs_move(c, 0, 0);
    float x1 = c->x + dx1, y1 = c->y + dy1;
    float x2 = x1 + dx2, y2 = y1 + dy2;
    c->x = x2 + dx3; c->y = y2 + dy3;
    float a, b, cc, d, e, ff;
    cs_pt(c, x1, y1, &a, &b);
    cs_pt(c, x2, y2, &cc, &d);
    cs_pt(c, c->x, c->y, &e, &ff);
    path_cubic(c->path, a, b, cc, d, e, ff);
}

static int cs_run(FontFile *f, CsCtx *c, const uint8_t *p, int len, float *s, int *spp, int depth) {
    const uint8_t *e = p + len;
    int sp = *spp;
    if (depth > 10) return 0;
    while (p < e) {
        int b0 = *p++;
        int i = 0;
        if (b0 >= 32 || b0 == 28) {
            float v;
            if (b0 == 28) { v = (float)(int16_t)rd16(p); p += 2; }
            else if (b0 <= 246) v = (float)(b0 - 139);
            else if (b0 <= 250) { v = (float)((b0 - 247) * 256 + *p + 108); p++; }
            else if (b0 <= 254) { v = (float)(-(b0 - 251) * 256 - *p - 108); p++; }
            else { v = (int32_t)rd32(p) / 65536.f; p += 4; }
            if (sp < 48) s[sp++] = v;
            continue;
        }
        switch (b0) {
        case 1: case 3: case 18: case 23: /* stems */
            c->nstems += sp / 2; sp = 0; break;
        case 19: case 20: /* hintmask / cntrmask */
            c->nstems += sp / 2; sp = 0;
            p += (c->nstems + 7) / 8;
            break;
        case 21: if (sp < 2) return 0; cs_move(c, s[sp - 2], s[sp - 1]); sp = 0; break;
        case 22: if (sp < 1) return 0; cs_move(c, s[sp - 1], 0); sp = 0; break;
        case 4: if (sp < 1) return 0; cs_move(c, 0, s[sp - 1]); sp = 0; break;
        case 5: for (; i + 1 < sp; i += 2) cs_line(c, s[i], s[i + 1]); sp = 0; break;
        case 6: case 7: {
            int horiz = b0 == 6;
            for (; i < sp; i++) { if (horiz) cs_line(c, s[i], 0); else cs_line(c, 0, s[i]); horiz = !horiz; }
            sp = 0; break;
        }
        case 8: for (; i + 5 < sp; i += 6) cs_curve(c, s[i], s[i + 1], s[i + 2], s[i + 3], s[i + 4], s[i + 5]); sp = 0; break;
        case 24: /* rcurveline */
            for (; i + 5 < sp - 2; i += 6) cs_curve(c, s[i], s[i + 1], s[i + 2], s[i + 3], s[i + 4], s[i + 5]);
            if (i + 1 < sp) cs_line(c, s[i], s[i + 1]);
            sp = 0; break;
        case 25: /* rlinecurve */
            for (; i + 1 < sp - 6; i += 2) cs_line(c, s[i], s[i + 1]);
            if (i + 5 < sp) cs_curve(c, s[i], s[i + 1], s[i + 2], s[i + 3], s[i + 4], s[i + 5]);
            sp = 0; break;
        case 26: case 27: { /* vvcurveto, hhcurveto */
            float fv = 0;
            if (sp & 1) { fv = s[0]; i = 1; }
            for (; i + 3 < sp; i += 4) {
                if (b0 == 27) cs_curve(c, s[i], fv, s[i + 1], s[i + 2], s[i + 3], 0);
                else cs_curve(c, fv, s[i], s[i + 1], s[i + 2], 0, s[i + 3]);
                fv = 0;
            }
            sp = 0; break;
        }
        case 30: case 31: { /* vhcurveto, hvcurveto */
            int h = b0 == 31;
            for (; i + 3 < sp; i += 4) {
                float last = (sp - i == 5) ? s[i + 4] : 0;
                if (h) cs_curve(c, s[i], 0, s[i + 1], s[i + 2], last, s[i + 3]);
                else cs_curve(c, 0, s[i], s[i + 1], s[i + 2], s[i + 3], last);
                h = !h;
            }
            sp = 0; break;
        }
        case 10: case 29: { /* callsubr, callgsubr */
            if (sp < 1) return 0;
            int idx = (int)s[--sp];
            const CffIndex *ix = b0 == 10 ? &f->subrs : &f->gsubrs;
            idx += b0 == 10 ? f->bias : f->gbias;
            int sl;
            const uint8_t *sub = cff_get(ix, idx, &sl);
            if (!sub) return 0;
            int r = cs_run(f, c, sub, sl, s, &sp, depth + 1);
            if (r == 2) { *spp = sp; return 2; }
            if (!r) return 0;
            break;
        }
        case 11: *spp = sp; return 1; /* return */
        case 14: /* endchar */
            if (c->started) path_close(c->path);
            *spp = 0;
            return 2;
        case 12: {
            int b1 = *p++;
            if (b1 == 35 && sp >= 13) {
                cs_curve(c, s[0], s[1], s[2], s[3], s[4], s[5]);
                cs_curve(c, s[6], s[7], s[8], s[9], s[10], s[11]);
            } else if (b1 == 34 && sp >= 7) {
                cs_curve(c, s[0], 0, s[1], s[2], s[3], 0);
                cs_curve(c, s[4], 0, s[5], -s[2], s[6], 0);
            } else if (b1 == 36 && sp >= 9) {
                cs_curve(c, s[0], s[1], s[2], s[3], s[4], 0);
                cs_curve(c, s[5], 0, s[6], s[7], s[8], -(s[1] + s[3] + s[7]));
            } else if (b1 == 37 && sp >= 11) {
                float dx = s[0] + s[2] + s[4] + s[6] + s[8], dy = s[1] + s[3] + s[5] + s[7] + s[9];
                float dx6, dy6;
                if (fabsf(dx) > fabsf(dy)) { dx6 = s[10]; dy6 = -dy; } else { dx6 = -dx; dy6 = s[10]; }
                cs_curve(c, s[0], s[1], s[2], s[3], s[4], s[5]);
                cs_curve(c, s[6], s[7], s[8], s[9], dx6, dy6);
            }
            sp = 0;
            break;
        }
        default: sp = 0; break;
        }
    }
    *spp = sp;
    return 1;
}

/* ---------------- glyf ---------------- */
static uint32_t glyf_offset(FontFile *f, int gid, uint32_t *len) {
    if (gid < 0 || gid >= f->nglyphs) return 0;
    uint32_t a, b;
    if (f->loc_long) { a = rd32(f->data + f->loca + 4 * gid); b = rd32(f->data + f->loca + 4 * gid + 4); }
    else { a = rd16(f->data + f->loca + 2 * gid) * 2; b = rd16(f->data + f->loca + 2 * gid + 2) * 2; }
    *len = b - a;
    return f->glyf + a;
}

static void glyf_outline(FontFile *f, int gid, Path *path, const float *m, int depth) {
    uint32_t len;
    uint32_t off = glyf_offset(f, gid, &len);
    if (!off || len == 0 || depth > 6) return;
    const uint8_t *g = f->data + off;
    int nc = rds16(g);
    if (nc >= 0) {
        const uint8_t *ends = g + 10;
        int npts = (int)rd16(ends + 2 * (nc - 1)) + 1;
        const uint8_t *ins = ends + 2 * nc;
        const uint8_t *fl = ins + 2 + rd16(ins);
        uint8_t *flags = xmalloc(npts);
        float *px = xmalloc(sizeof(float) * npts * 2), *py = px + npts;
        for (int i = 0; i < npts;) {
            uint8_t fg = *fl++;
            flags[i++] = fg;
            if (fg & 8) { int r = *fl++; while (r-- > 0 && i < npts) flags[i++] = fg; }
        }
        int v = 0;
        for (int i = 0; i < npts; i++) {
            uint8_t fg = flags[i];
            if (fg & 2) { int d = *fl++; v += (fg & 16) ? d : -d; }
            else if (!(fg & 16)) { v += rds16(fl); fl += 2; }
            px[i] = (float)v;
        }
        v = 0;
        for (int i = 0; i < npts; i++) {
            uint8_t fg = flags[i];
            if (fg & 4) { int d = *fl++; v += (fg & 32) ? d : -d; }
            else if (!(fg & 32)) { v += rds16(fl); fl += 2; }
            py[i] = (float)v;
        }
        for (int i = 0; i < npts; i++) {
            float x = px[i], y = py[i];
            px[i] = m[0] * x + m[1] * y + m[2];
            py[i] = m[3] * x + m[4] * y + m[5];
        }
        int start = 0;
        for (int ci = 0; ci < nc; ci++) {
            int end = (int)rd16(ends + 2 * ci);
            int n = end - start + 1;
            if (n < 2) { start = end + 1; continue; }
#define ON(k) (flags[start + ((k) % n)] & 1)
#define PX(k) px[start + ((k) % n)]
#define PY(k) py[start + ((k) % n)]
            int first = 0;
            float sx, sy;
            if (ON(0)) { sx = PX(0); sy = PY(0); first = 1; }
            else if (ON(n - 1)) { sx = PX(n - 1); sy = PY(n - 1); first = 0; n--; }
            else { sx = (PX(0) + PX(n - 1)) * 0.5f; sy = (PY(0) + PY(n - 1)) * 0.5f; first = 0; }
            path_move(path, sx, sy);
            int haveCtrl = 0;
            float cxp = 0, cyp = 0;
            for (int k = first; k < n + (first ? 0 : 0); k++) {
                float x = PX(k), y = PY(k);
                if (ON(k)) {
                    if (haveCtrl) path_quad(path, cxp, cyp, x, y); else path_line(path, x, y);
                    haveCtrl = 0;
                } else {
                    if (haveCtrl) {
                        float mx = (cxp + x) * 0.5f, my = (cyp + y) * 0.5f;
                        path_quad(path, cxp, cyp, mx, my);
                    }
                    cxp = x; cyp = y; haveCtrl = 1;
                }
            }
            if (haveCtrl) path_quad(path, cxp, cyp, sx, sy);
            path_close(path);
#undef ON
#undef PX
#undef PY
            start = end + 1;
        }
        free(flags);
        free(px);
    } else {
        const uint8_t *p = g + 10;
        for (;;) {
            int fl = (int)rd16(p), cg = (int)rd16(p + 2);
            p += 4;
            float dx, dy;
            if (fl & 1) { dx = (float)rds16(p); dy = (float)rds16(p + 2); p += 4; }
            else { dx = (float)(int8_t)p[0]; dy = (float)(int8_t)p[1]; p += 2; }
            float a = 1, b = 0, c = 0, d = 1;
            if (fl & 8) { a = d = rds16(p) / 16384.f; p += 2; }
            else if (fl & 0x40) { a = rds16(p) / 16384.f; d = rds16(p + 2) / 16384.f; p += 4; }
            else if (fl & 0x80) { a = rds16(p) / 16384.f; b = rds16(p + 2) / 16384.f; c = rds16(p + 4) / 16384.f; d = rds16(p + 6) / 16384.f; p += 8; }
            if (!(fl & 2)) { dx = dy = 0; } /* point matching unsupported */
            /* child -> parent: x' = a x + c y + dx ; y' = b x + d y + dy */
            float cm[6] = {
                m[0] * a + m[1] * b, m[0] * c + m[1] * d, m[0] * dx + m[1] * dy + m[2],
                m[3] * a + m[4] * b, m[3] * c + m[4] * d, m[3] * dx + m[4] * dy + m[5],
            };
            glyf_outline(f, cg, path, cm, depth + 1);
            if (!(fl & 0x20)) break;
        }
    }
}

static void glyph_path(FontFile *f, int gid, Path *path, float scale) {
    float m[6] = {scale, 0, 0, 0, -scale, 0};
    if (f->is_cff) {
        int len;
        const uint8_t *cs = cff_get(&f->charstrings, gid, &len);
        if (!cs) return;
        CsCtx c;
        memset(&c, 0, sizeof c);
        c.path = path;
        memcpy(c.tx, m, sizeof m);
        float st[48];
        int sp = 0;
        cs_run(f, &c, cs, len, st, &sp, 0);
        if (path->open) path_close(path);
    } else {
        glyf_outline(f, gid, path, m, 0);
    }
}

/* ---------------- cmap ---------------- */
static int cmap_lookup(FontFile *f, uint32_t cp) {
    if (cp < 128 && f->ascii_gid[cp]) return f->ascii_gid[cp] == 0xFFFF ? 0 : f->ascii_gid[cp];
    const uint8_t *t = f->data + f->cmap;
    int gid = 0;
    if (f->cmap_fmt == 4) {
        if (cp > 0xFFFF) return 0;
        int segx2 = (int)rd16(t + 6);
        const uint8_t *ends = t + 14, *starts = ends + segx2 + 2, *deltas = starts + segx2, *ranges = deltas + segx2;
        int lo = 0, hi = segx2 / 2 - 1;
        while (lo <= hi) {
            int mid = (lo + hi) / 2;
            uint32_t end = rd16(ends + 2 * mid);
            if (cp > end) lo = mid + 1;
            else {
                uint32_t start = rd16(starts + 2 * mid);
                if (cp < start) { hi = mid - 1; continue; }
                uint32_t ro = rd16(ranges + 2 * mid);
                if (ro == 0) gid = (int)((cp + rd16(deltas + 2 * mid)) & 0xFFFF);
                else {
                    uint32_t g = rd16(ranges + 2 * mid + ro + 2 * (cp - start));
                    gid = g ? (int)((g + rd16(deltas + 2 * mid)) & 0xFFFF) : 0;
                }
                break;
            }
        }
    } else if (f->cmap_fmt == 12) {
        uint32_t n = rd32(t + 12);
        uint32_t lo = 0, hi = n;
        while (lo < hi) {
            uint32_t mid = (lo + hi) / 2;
            const uint8_t *gp = t + 16 + 12 * mid;
            uint32_t s = rd32(gp), e = rd32(gp + 4);
            if (cp < s) hi = mid;
            else if (cp > e) lo = mid + 1;
            else { gid = (int)(rd32(gp + 8) + cp - s); break; }
        }
    }
    if (cp < 128) f->ascii_gid[cp] = gid ? (uint16_t)gid : 0xFFFF;
    return gid;
}

static float advance(FontFile *f, int gid) {
    int i = gid < f->nhmetrics ? gid : f->nhmetrics - 1;
    return (float)rd16(f->data + f->hmtx + 4 * i);
}

static int font_open(FontFile *f, const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return 0;
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (sz < 64) { fclose(fp); return 0; }
    f->data = xmalloc((size_t)sz);
    if (fread(f->data, 1, (size_t)sz, fp) != (size_t)sz) { fclose(fp); free(f->data); return 0; }
    fclose(fp);
    f->size = (size_t)sz;
    const uint8_t *d = f->data;
    uint32_t head = find_table(d, "head"), hhea = find_table(d, "hhea"), cmap = find_table(d, "cmap");
    uint32_t maxp = find_table(d, "maxp");
    f->hmtx = find_table(d, "hmtx");
    if (!head || !hhea || !cmap || !maxp || !f->hmtx) { free(f->data); return 0; }
    f->upem = (int)rd16(d + head + 18);
    f->loc_long = rds16(d + head + 50);
    f->nglyphs = (int)rd16(d + maxp + 4);
    f->ascender = rds16(d + hhea + 4);
    f->descender = rds16(d + hhea + 6);
    f->nhmetrics = (int)rd16(d + hhea + 34);
    int nsub = (int)rd16(d + cmap + 2);
    uint32_t best = 0;
    int bestfmt = 0, bestscore = -1;
    for (int i = 0; i < nsub; i++) {
        const uint8_t *r = d + cmap + 4 + 8 * i;
        int pid = (int)rd16(r), eid = (int)rd16(r + 2);
        uint32_t off = cmap + rd32(r + 4);
        int fmt = (int)rd16(d + off);
        int score = -1;
        if (fmt == 12 && (pid == 3 || pid == 0)) score = 3;
        else if (fmt == 4 && ((pid == 3 && (eid == 1 || eid == 10)) || pid == 0)) score = 2;
        if (score > bestscore) { bestscore = score; best = off; bestfmt = fmt; }
    }
    if (bestscore < 0) { free(f->data); return 0; }
    f->cmap = best;
    f->cmap_fmt = bestfmt;
    uint32_t cff = find_table(d, "CFF ");
    if (cff) {
        f->is_cff = 1;
        if (!cff_init(f, cff)) { free(f->data); return 0; }
    } else {
        f->loca = find_table(d, "loca");
        f->glyf = find_table(d, "glyf");
        if (!f->loca || !f->glyf) { free(f->data); return 0; }
    }
    /* Cap height measured from the outline of 'H'. */
    Path p;
    path_init(&p);
    glyph_path(f, cmap_lookup(f, 'H'), &p, 1.f);
    float top = 0;
    for (int i = 0; i < p.nseg; i++) top = fminf(top, fminf(p.seg[4 * i + 1], p.seg[4 * i + 3]));
    path_free(&p);
    f->cap = top < 0 ? -top : f->upem * 0.7f;
    return 1;
}

/* ---------------- Font discovery ---------------- */
static int find_file(const char *dir, const char *name, char *out, int depth) {
    if (depth > 4) return 0;
    DIR *d = opendir(dir);
    if (!d) return 0;
    struct dirent *e;
    int found = 0;
    char sub[1024];
    while (!found && (e = readdir(d))) {
        if (e->d_name[0] == '.') continue;
        snprintf(sub, sizeof sub, "%s/%s", dir, e->d_name);
        if (strcmp(e->d_name, name) == 0) { snprintf(out, 512, "%s", sub); found = 1; break; }
        struct stat st;
        if (stat(sub, &st) == 0 && S_ISDIR(st.st_mode)) found = find_file(sub, name, out, depth + 1);
    }
    closedir(d);
    return found;
}

static const char *const g_candidates[FONT_COUNT][6] = {
    [FONT_THIN] = {"InterDisplay-Light.otf", "Inter-Light.otf", "DejaVuSans-ExtraLight.ttf", "DejaVuSans.ttf", "LiberationSans-Regular.ttf", NULL},
    [FONT_LIGHT] = {"Inter-Light.otf", "InterDisplay-Light.otf", "DejaVuSans.ttf", "LiberationSans-Regular.ttf", NULL},
    [FONT_REGULAR] = {"Inter-Regular.otf", "DejaVuSans.ttf", "LiberationSans-Regular.ttf", "FreeSans.ttf", NULL},
    [FONT_MEDIUM] = {"Inter-Medium.otf", "DejaVuSans.ttf", "LiberationSans-Regular.ttf", NULL},
    [FONT_SEMIBOLD] = {"Inter-SemiBold.otf", "DejaVuSans-Bold.ttf", "LiberationSans-Bold.ttf", NULL},
    [FONT_BOLD] = {"Inter-Bold.otf", "DejaVuSans-Bold.ttf", "LiberationSans-Bold.ttf", "FreeSansBold.ttf", NULL},
    [FONT_DISPLAY] = {"InterDisplay-Bold.otf", "Inter-Bold.otf", "DejaVuSans-Bold.ttf", "LiberationSans-Bold.ttf", NULL},
    [FONT_BLACK] = {"InterDisplay-Black.otf", "Inter-Black.otf", "InterDisplay-ExtraBold.otf", "DejaVuSans-Bold.ttf", "LiberationSans-Bold.ttf", NULL},
};

int fonts_init(void) {
    const char *dirs[] = {getenv("KEEL_FONT_DIR"), "/usr/share/fonts", "/usr/local/share/fonts", "/system/fonts"};
    int ok = 1;
    for (int i = 0; i < FONT_COUNT; i++) {
        int got = 0;
        for (int c = 0; g_candidates[i][c] && !got; c++) {
            for (int d = 0; d < 4 && !got; d++) {
                if (!dirs[d]) continue;
                char path[512];
                if (find_file(dirs[d], g_candidates[i][c], path, 0) && font_open(&g_fonts[i], path)) {
                    snprintf(g_font_paths[i], sizeof g_font_paths[i], "%s", path);
                    got = 1;
                }
            }
        }
        if (!got) {
            /* Fall back to any already-loaded face. */
            for (int j = 0; j < i && !got; j++)
                if (g_fonts[j].data) { g_fonts[i] = g_fonts[j]; snprintf(g_font_paths[i], 512, "%s", g_font_paths[j]); got = 1; }
        }
        if (!got) ok = 0;
    }
    if (!ok) {
        for (int i = FONT_COUNT - 1; i >= 0; i--)
            if (g_fonts[i].data) { for (int j = 0; j < FONT_COUNT; j++) if (!g_fonts[j].data) { g_fonts[j] = g_fonts[i]; snprintf(g_font_paths[j], 512, "%s", g_font_paths[i]); } ok = 1; break; }
    }
    return ok;
}
const char *font_path(int font) { return g_font_paths[font]; }
int font_glyph_count(int font) { return g_fonts[font].nglyphs; }

/* ---------------- Glyph cache ---------------- */
typedef struct { Mask *m; int ox, oy; float adv; } Glyph;
typedef struct { uint64_t key; Glyph *g; } GEntry;
#define GCAP 16384
static GEntry g_cache[GCAP];
static int g_cache_n;

static const Glyph *glyph_get(int font, int px, int gid) {
    uint64_t key = ((uint64_t)(font + 1) << 48) | ((uint64_t)px << 24) | (uint64_t)gid;
    uint32_t h = (uint32_t)(key * 0x9E3779B97F4A7C15ull >> 40) & (GCAP - 1);
    pthread_mutex_lock(&g_glyph_lock);
    for (;;) {
        if (g_cache[h].key == key) { Glyph *g = g_cache[h].g; pthread_mutex_unlock(&g_glyph_lock); return g; }
        if (!g_cache[h].key) break;
        h = (h + 1) & (GCAP - 1);
    }
    pthread_mutex_unlock(&g_glyph_lock);
    FontFile *f = &g_fonts[font];
    float scale = (float)px / f->upem;
    Glyph *g = xcalloc(1, sizeof *g);
    Path p;
    path_init(&p);
    glyph_path(f, gid, &p, scale);
    /* Slight gamma lift keeps light text crisp on bright imagery. */
    g->m = path_to_mask(&p, &g->ox, &g->oy, px < 30 ? 0.82f : 0.92f);
    path_free(&p);
    g->adv = advance(f, gid) * scale;
    pthread_mutex_lock(&g_glyph_lock);
    /* Another thread may have inserted the same glyph meanwhile. */
    for (;;) {
        if (g_cache[h].key == key) { mask_free(g->m); free(g); g = g_cache[h].g; break; }
        if (!g_cache[h].key) {
            if (g_cache_n < GCAP * 3 / 4) { g_cache[h].key = key; g_cache[h].g = g; g_cache_n++; }
            break;
        }
        h = (h + 1) & (GCAP - 1);
    }
    pthread_mutex_unlock(&g_glyph_lock);
    return g;
}

static uint32_t utf8_next(const char **s) {
    const unsigned char *p = (const unsigned char *)*s;
    uint32_t c = *p++;
    if (c >= 0xF0 && p[0] && p[1] && p[2]) { c = ((c & 7) << 18) | ((p[0] & 63) << 12) | ((p[1] & 63) << 6) | (p[2] & 63); p += 3; }
    else if (c >= 0xE0 && p[0] && p[1]) { c = ((c & 15) << 12) | ((p[0] & 63) << 6) | (p[1] & 63); p += 2; }
    else if (c >= 0xC0 && p[0]) { c = ((c & 31) << 6) | (p[0] & 63); p += 1; }
    *s = (const char *)p;
    return c;
}

float font_cap(int font, float size) { FontFile *f = &g_fonts[font]; return f->cap / f->upem * size; }
float font_ascent(int font, float size) { FontFile *f = &g_fonts[font]; return (float)f->ascender / f->upem * size; }

float text_width(int font, float size, const char *s, float tracking) {
    FontFile *f = &g_fonts[font];
    if (!f->data || !s) return 0;
    float scale = size / f->upem, w = 0;
    int n = 0;
    while (*s) {
        uint32_t cp = utf8_next(&s);
        w += advance(f, cmap_lookup(f, cp)) * scale + tracking;
        n++;
    }
    return n ? w - tracking : 0;
}

static float draw_run(Gfx *g, int font, float size, float x, float y, const char *s, int nbytes, uint32_t c, float tracking) {
    FontFile *f = &g_fonts[font];
    if (!f->data) return 0;
    int px = (int)(size + 0.5f);
    float base = y + roundf(font_cap(font, (float)px));
    GlyphPos stackbuf[256];
    int cap = 256, n = 0;
    GlyphPos *gp = stackbuf;
    float pen = x;
    const char *end = s + nbytes;
    while (s < end && *s) {
        uint32_t cp = utf8_next(&s);
        int gid = cmap_lookup(f, cp);
        const Glyph *gl = glyph_get(font, px, gid);
        if (n == cap) {
            GlyphPos *nb = xmalloc(sizeof(GlyphPos) * cap * 2);
            memcpy(nb, gp, sizeof(GlyphPos) * n);
            if (gp != stackbuf) free(gp);
            gp = nb; cap *= 2;
        }
        if (gl->m && gl->m->a && gl->m->w > 0) {
            gp[n].m = gl->m;
            gp[n].x = (int)floorf(pen + 0.5f) + gl->ox;
            gp[n].y = (int)base + gl->oy;
            n++;
        }
        pen += gl->adv + tracking;
    }
    g_glyphs(g, gp, n, c);
    if (gp != stackbuf) free(gp);
    return pen - x - tracking;
}

float g_text(Gfx *g, int font, float size, float x, float y, const char *s, uint32_t c, int align, float tracking) {
    if (!s || !*s) return 0;
    float w = 0;
    if (align != ALIGN_LEFT) {
        w = text_width(font, size, s, tracking);
        x -= align == ALIGN_CENTER ? w * 0.5f : w;
    }
    return draw_run(g, font, size, x, y, s, (int)strlen(s), c, tracking);
}

float g_text_shadow(Gfx *g, int font, float size, float x, float y, const char *s, uint32_t c, int align, float tracking) {
    float a = col_a(c) / 255.f;
    float off = fmaxf(1.f, size * 0.04f);
    g_text(g, font, size, x, y + off * 1.5f, s, col_alpha(0xFF000000, 0.22f * a), align, tracking);
    g_text(g, font, size, x, y + off * 0.6f, s, col_alpha(0xFF000000, 0.28f * a), align, tracking);
    return g_text(g, font, size, x, y, s, c, align, tracking);
}

float g_text_fit(Gfx *g, int font, float size, float x, float y, const char *s, uint32_t c, float max_w, int align) {
    if (!s) return 0;
    float w = text_width(font, size, s, 0);
    if (w <= max_w) return g_text(g, font, size, x, y, s, c, align, 0);
    float ew = text_width(font, size, "\xE2\x80\xA6", 0);
    char buf[512];
    int len = (int)strlen(s);
    if (len > 500) len = 500;
    while (len > 0) {
        len--;
        while (len > 0 && ((unsigned char)s[len] & 0xC0) == 0x80) len--;
        memcpy(buf, s, len);
        buf[len] = 0;
        if (text_width(font, size, buf, 0) + ew <= max_w) break;
    }
    while (len > 0 && buf[len - 1] == ' ') buf[--len] = 0;
    strcat(buf, "\xE2\x80\xA6");
    return g_text(g, font, size, x, y, buf, c, align, 0);
}

int g_text_wrap(Gfx *g, int font, float size, float x, float y, float max_w, float line_h, int max_lines,
                const char *s, uint32_t c) {
    int lines = 0;
    char line[1024];
    while (*s && lines < max_lines) {
        while (*s == ' ') s++;
        int best = 0, i = 0;
        int len = (int)strlen(s);
        for (;;) {
            while (i < len && s[i] != ' ') i++;
            if (i > 1000) break;
            memcpy(line, s, i);
            line[i] = 0;
            if (text_width(font, size, line, 0) > max_w && best > 0) break;
            best = i;
            if (i >= len) break;
            i++;
        }
        if (best == 0) best = MINI(len, 1000);
        int last = lines == max_lines - 1 && best < len;
        memcpy(line, s, best);
        line[best] = 0;
        if (last) g_text_fit(g, font, size, x, y + lines * line_h, s, c, max_w, ALIGN_LEFT);
        else g_text(g, font, size, x, y + lines * line_h, line, c, ALIGN_LEFT, 0);
        s += best;
        lines++;
    }
    return lines;
}
