/* KEEL OS — anti-aliased vector path rasterizer.
 * Exact-area coverage accumulation: each edge deposits its signed area into
 * an accumulation buffer, a running sum per row yields coverage (non-zero,
 * saturated). Curves are flattened adaptively. */
#include "raster.h"

void path_init(Path *p) {
    memset(p, 0, sizeof *p);
    path_identity(p);
}
void path_free(Path *p) { free(p->seg); p->seg = NULL; p->nseg = p->cap = 0; }
void path_reset(Path *p) { p->nseg = 0; p->open = 0; }
void path_identity(Path *p) { float m[6] = {1, 0, 0, 0, 1, 0}; memcpy(p->m, m, sizeof m); }
void path_transform(Path *p, float a, float b, float c, float d, float e, float f) {
    p->m[0] = a; p->m[1] = b; p->m[2] = c; p->m[3] = d; p->m[4] = e; p->m[5] = f;
}
static inline void xf(const Path *p, float x, float y, float *ox, float *oy) {
    *ox = p->m[0] * x + p->m[1] * y + p->m[2];
    *oy = p->m[3] * x + p->m[4] * y + p->m[5];
}
static void add_seg(Path *p, float x0, float y0, float x1, float y1) {
    if (y0 == y1) return; /* horizontal edges deposit no area */
    if (p->nseg == p->cap) {
        p->cap = p->cap ? p->cap * 2 : 256;
        p->seg = realloc(p->seg, sizeof(float) * 4 * p->cap);
        if (!p->seg) abort();
    }
    float *s = p->seg + 4 * p->nseg++;
    s[0] = x0; s[1] = y0; s[2] = x1; s[3] = y1;
}
void path_move(Path *p, float x, float y) {
    if (p->open) path_close(p);
    xf(p, x, y, &p->cx, &p->cy);
    p->sx = p->cx; p->sy = p->cy;
    p->open = 1;
}
static void line_abs(Path *p, float x, float y) {
    add_seg(p, p->cx, p->cy, x, y);
    p->cx = x; p->cy = y;
}
void path_line(Path *p, float x, float y) {
    float tx, ty;
    xf(p, x, y, &tx, &ty);
    line_abs(p, tx, ty);
}
void path_quad(Path *p, float x1, float y1, float x2, float y2) {
    float ax, ay, bx, by;
    xf(p, x1, y1, &ax, &ay);
    xf(p, x2, y2, &bx, &by);
    float x0 = p->cx, y0 = p->cy;
    float dd = fabsf(x0 - 2 * ax + bx) + fabsf(y0 - 2 * ay + by);
    int n = 1 + (int)sqrtf(dd * 3.f);
    if (n > 64) n = 64;
    for (int i = 1; i <= n; i++) {
        float t = (float)i / n, u = 1 - t;
        line_abs(p, u * u * x0 + 2 * u * t * ax + t * t * bx, u * u * y0 + 2 * u * t * ay + t * t * by);
    }
}
void path_cubic(Path *p, float x1, float y1, float x2, float y2, float x3, float y3) {
    float ax, ay, bx, by, cx, cy;
    xf(p, x1, y1, &ax, &ay);
    xf(p, x2, y2, &bx, &by);
    xf(p, x3, y3, &cx, &cy);
    float x0 = p->cx, y0 = p->cy;
    float dd = fabsf(x0 - 2 * ax + bx) + fabsf(y0 - 2 * ay + by) + fabsf(ax - 2 * bx + cx) + fabsf(ay - 2 * by + cy);
    int n = 1 + (int)sqrtf(dd * 3.f);
    if (n > 96) n = 96;
    for (int i = 1; i <= n; i++) {
        float t = (float)i / n, u = 1 - t;
        float a = u * u * u, b = 3 * u * u * t, c = 3 * u * t * t, d = t * t * t;
        line_abs(p, a * x0 + b * ax + c * bx + d * cx, a * y0 + b * ay + c * by + d * cy);
    }
}
void path_close(Path *p) {
    if (!p->open) return;
    line_abs(p, p->sx, p->sy);
    p->open = 0;
}
void path_rect(Path *p, float x, float y, float w, float h) {
    path_move(p, x, y); path_line(p, x + w, y); path_line(p, x + w, y + h); path_line(p, x, y + h); path_close(p);
}
void path_rrect(Path *p, float x, float y, float w, float h, float r) {
    r = fminf(r, fminf(w, h) * 0.5f);
    if (r <= 0) { path_rect(p, x, y, w, h); return; }
    const float k = 0.5523f * r;
    path_move(p, x + r, y);
    path_line(p, x + w - r, y);
    path_cubic(p, x + w - r + k, y, x + w, y + r - k, x + w, y + r);
    path_line(p, x + w, y + h - r);
    path_cubic(p, x + w, y + h - r + k, x + w - r + k, y + h, x + w - r, y + h);
    path_line(p, x + r, y + h);
    path_cubic(p, x + r - k, y + h, x, y + h - r + k, x, y + h - r);
    path_line(p, x, y + r);
    path_cubic(p, x, y + r - k, x + r - k, y, x + r, y);
    path_close(p);
}
void path_ellipse(Path *p, float cx, float cy, float rx, float ry, float rot) {
    int n = 16 + (int)(sqrtf(fmaxf(rx, ry)) * 6.f);
    if (n > 256) n = 256;
    float c = cosf(rot), s = sinf(rot);
    for (int i = 0; i <= n; i++) {
        float a = 2 * PI_F * i / n;
        float ex = cosf(a) * rx, ey = sinf(a) * ry;
        float x = cx + ex * c - ey * s, y = cy + ex * s + ey * c;
        if (i == 0) path_move(p, x, y); else path_line(p, x, y);
    }
    path_close(p);
}
void path_circle(Path *p, float cx, float cy, float r) { path_ellipse(p, cx, cy, r, r, 0); }
void path_poly(Path *p, const float *xy, int n) {
    for (int i = 0; i < n; i++) {
        if (i == 0) path_move(p, xy[0], xy[1]); else path_line(p, xy[2 * i], xy[2 * i + 1]);
    }
    path_close(p);
}
/* Oriented quad / circle so the union never cancels. */
static void quad_cw(Path *p, float ax, float ay, float bx, float by, float cx, float cy, float dx, float dy) {
    float area = (bx - ax) * (cy - ay) - (cx - ax) * (by - ay);
    if (area >= 0) { path_move(p, ax, ay); path_line(p, bx, by); path_line(p, cx, cy); path_line(p, dx, dy); }
    else { path_move(p, ax, ay); path_line(p, dx, dy); path_line(p, cx, cy); path_line(p, bx, by); }
    path_close(p);
}
static void circle_cw(Path *p, float cx, float cy, float r) {
    int n = 12 + (int)(sqrtf(r) * 4);
    if (n > 128) n = 128;
    for (int i = 0; i <= n; i++) {
        float a = 2 * PI_F * i / n;
        float x = cx + cosf(a) * r, y = cy + sinf(a) * r;
        if (i == 0) path_move(p, x, y); else path_line(p, x, y);
    }
    path_close(p);
}
void path_stroke(Path *p, const float *xy, int n, float width, int closed) {
    float hw = width * 0.5f;
    int segs = closed ? n : n - 1;
    for (int i = 0; i < segs; i++) {
        float x0 = xy[2 * i], y0 = xy[2 * i + 1];
        float x1 = xy[2 * ((i + 1) % n)], y1 = xy[2 * ((i + 1) % n) + 1];
        float dx = x1 - x0, dy = y1 - y0, l = sqrtf(dx * dx + dy * dy);
        if (l < 1e-5f) continue;
        float nx = -dy / l * hw, ny = dx / l * hw;
        quad_cw(p, x0 + nx, y0 + ny, x1 + nx, y1 + ny, x1 - nx, y1 - ny, x0 - nx, y0 - ny);
    }
    for (int i = 0; i < n; i++) circle_cw(p, xy[2 * i], xy[2 * i + 1], hw);
}
void path_arc_stroke(Path *p, float cx, float cy, float r, float a0, float a1, float width) {
    int n = 8 + (int)(fabsf(a1 - a0) * r / 6.f);
    if (n > 200) n = 200;
    float *pts = xmalloc(sizeof(float) * 2 * (n + 1));
    for (int i = 0; i <= n; i++) {
        float a = a0 + (a1 - a0) * i / n;
        pts[2 * i] = cx + cosf(a) * r; pts[2 * i + 1] = cy + sinf(a) * r;
    }
    path_stroke(p, pts, n + 1, width, 0);
    free(pts);
}

/* ---- Accumulation ---- */
typedef struct { float *acc; int w, h; float ox, oy; } Acc;

static void acc_line(Acc *a, float x0, float y0, float x1, float y1) {
    x0 -= a->ox; x1 -= a->ox; y0 -= a->oy; y1 -= a->oy;
    float dir = 1.f;
    if (y0 > y1) { float t = x0; x0 = x1; x1 = t; t = y0; y0 = y1; y1 = t; dir = -1.f; }
    if (y1 <= 0 || y0 >= a->h) return;
    float dxdy = (x1 - x0) / (y1 - y0);
    float x = x0;
    if (y0 < 0) { x -= y0 * dxdy; y0 = 0; }
    if (y1 > a->h) y1 = (float)a->h;
    const float wmax = (float)a->w;
    int ys = (int)y0, ye = (int)ceilf(y1);
    for (int y = ys; y < ye; y++) {
        float *row = a->acc + (size_t)y * (a->w + 2);
        float dy = fminf((float)(y + 1), y1) - fmaxf((float)y, y0);
        float xn = x + dxdy * dy;
        float d = dy * dir;
        float xa = fminf(x, xn), xb = fmaxf(x, xn);
        xa = clampf(xa, 0, wmax); xb = clampf(xb, 0, wmax);
        float xaf = floorf(xa);
        int xai = (int)xaf;
        float xbc = ceilf(xb);
        int xbi = (int)xbc;
        if (xbi <= xai + 1) {
            float xm = 0.5f * (xa + xb) - xaf;
            row[xai] += d - d * xm;
            row[xai + 1] += d * xm;
        } else {
            float s = 1.f / (xb - xa);
            float x0f = xa - xaf;
            float a0 = 0.5f * s * (1 - x0f) * (1 - x0f);
            float x1f = xb - xbc + 1;
            float am = 0.5f * s * x1f * x1f;
            row[xai] += d * a0;
            if (xbi == xai + 2) row[xai + 1] += d * (1 - a0 - am);
            else {
                float a1 = s * (1.5f - x0f);
                row[xai + 1] += d * (a1 - a0);
                for (int xi = xai + 2; xi < xbi - 1; xi++) row[xi] += d * s;
                float a2 = a1 + (xbi - xai - 3) * s;
                row[xbi - 1] += d * (1 - a2 - am);
            }
            row[xbi] += d * am;
        }
        x = xn;
    }
}

static int path_bbox(const Path *p, float *x0, float *y0, float *x1, float *y1) {
    if (!p->nseg) return 0;
    *x0 = *y0 = 1e30f; *x1 = *y1 = -1e30f;
    for (int i = 0; i < p->nseg; i++) {
        const float *s = p->seg + 4 * i;
        *x0 = fminf(*x0, fminf(s[0], s[2])); *x1 = fmaxf(*x1, fmaxf(s[0], s[2]));
        *y0 = fminf(*y0, fminf(s[1], s[3])); *y1 = fmaxf(*y1, fmaxf(s[1], s[3]));
    }
    return 1;
}

static void acc_fill(Acc *a, const Path *p) {
    for (int i = 0; i < p->nseg; i++) {
        const float *s = p->seg + 4 * i;
        acc_line(a, s[0], s[1], s[2], s[3]);
    }
}

void path_fill(Surface *s, const Path *p, const Paint *paint) {
    float bx0, by0, bx1, by1;
    if (!path_bbox(p, &bx0, &by0, &bx1, &by1)) return;
    int ix0 = MAXI(0, (int)floorf(bx0)), iy0 = MAXI(0, (int)floorf(by0));
    int ix1 = MINI(s->w, (int)ceilf(bx1) + 1), iy1 = MINI(s->h, (int)ceilf(by1) + 1);
    if (ix0 >= ix1 || iy0 >= iy1) return;
    Acc a = {NULL, ix1 - ix0, iy1 - iy0, (float)ix0, (float)iy0};
    a.acc = xcalloc((size_t)(a.w + 2) * a.h, sizeof(float));
    acc_fill(&a, p);
    float gdx = 0, gdy = 0, rinv = 0;
    if (paint->type == PAINT_LINEAR) {
        float dx = paint->x1 - paint->x0, dy = paint->y1 - paint->y0, l2 = dx * dx + dy * dy;
        if (l2 > 0) { gdx = dx / l2; gdy = dy / l2; }
    } else if (paint->type == PAINT_RADIAL) rinv = paint->r > 0 ? 1.f / paint->r : 0;
    for (int y = 0; y < a.h; y++) {
        float *row = a.acc + (size_t)y * (a.w + 2);
        uint32_t *dst = s->px + (size_t)(y + iy0) * s->stride + ix0;
        float sum = 0;
        float py = y + iy0 + 0.5f;
        for (int x = 0; x < a.w; x++) {
            sum += row[x];
            float c = fabsf(sum);
            if (c < 1.f / 512) continue;
            uint32_t cov = c >= 1.f ? 255 : (uint32_t)(c * 255.f + 0.5f);
            uint32_t col = paint->c0;
            if (paint->type == PAINT_LINEAR) {
                float t = (x + ix0 + 0.5f - paint->x0) * gdx + (py - paint->y0) * gdy;
                col = lerp256(paint->c0, paint->c1, (uint32_t)(sat(t) * 256.f));
            } else if (paint->type == PAINT_RADIAL) {
                float dx = x + ix0 + 0.5f - paint->x0, dy = py - paint->y0;
                float t = sqrtf(dx * dx + dy * dy) * rinv;
                col = lerp256(paint->c0, paint->c1, (uint32_t)(sat(t) * 256.f));
            }
            dst[x] = blend(dst[x], cov >= 255 ? col : mul8(col, cov));
        }
    }
    free(a.acc);
}

static uint8_t *acc_to_a8(Acc *a, float gamma) {
    uint8_t lut[257];
    for (int i = 0; i <= 256; i++) lut[i] = (uint8_t)(powf(i / 256.f, gamma) * 255.f + 0.5f);
    uint8_t *out = xcalloc((size_t)a->w * a->h, 1);
    for (int y = 0; y < a->h; y++) {
        float *row = a->acc + (size_t)y * (a->w + 2);
        float sum = 0;
        for (int x = 0; x < a->w; x++) {
            sum += row[x];
            float c = fabsf(sum);
            int i = c >= 1.f ? 256 : (int)(c * 256.f);
            out[(size_t)y * a->w + x] = lut[i];
        }
    }
    return out;
}

Mask *path_to_mask(const Path *p, int *ox, int *oy, float gamma) {
    float bx0, by0, bx1, by1;
    Mask *m = xcalloc(1, sizeof *m);
    if (!path_bbox(p, &bx0, &by0, &bx1, &by1)) { *ox = *oy = 0; return m; }
    int ix0 = (int)floorf(bx0), iy0 = (int)floorf(by0);
    int ix1 = (int)ceilf(bx1) + 1, iy1 = (int)ceilf(by1) + 1;
    Acc a = {NULL, ix1 - ix0, iy1 - iy0, (float)ix0, (float)iy0};
    a.acc = xcalloc((size_t)(a.w + 2) * a.h, sizeof(float));
    acc_fill(&a, p);
    m->w = a.w; m->h = a.h;
    m->a = acc_to_a8(&a, gamma);
    free(a.acc);
    *ox = ix0; *oy = iy0;
    return m;
}
Mask *path_to_mask_fixed(const Path *p, int w, int h) {
    Mask *m = xcalloc(1, sizeof *m);
    Acc a = {NULL, w, h, 0, 0};
    a.acc = xcalloc((size_t)(w + 2) * h, sizeof(float));
    acc_fill(&a, p);
    m->w = w; m->h = h;
    m->a = acc_to_a8(&a, 1.f);
    free(a.acc);
    return m;
}
void mask_free(Mask *m) {
    if (!m) return;
    free(m->a);
    free(m);
}
