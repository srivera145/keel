/* KEEL OS — vector icon set. Each icon is built from unit-space paths: an
 * additive path and an optional subtractive one (holes, cut-outs). */
#include "icons.h"
#include "assets.h"
#include "raster.h"
#include <pthread.h>

typedef struct { Path *add, *sub; float x, y, s; } IB;

static void tf(IB *b, Path *p) { path_transform(p, b->s, 0, b->x, 0, b->s, b->y); }
static void A_rrect(IB *b, float x, float y, float w, float h, float r) { tf(b, b->add); path_rrect(b->add, x, y, w, h, r); }
static void S_rrect(IB *b, float x, float y, float w, float h, float r) { tf(b, b->sub); path_rrect(b->sub, x, y, w, h, r); }
static void A_circle(IB *b, float x, float y, float r) { tf(b, b->add); path_circle(b->add, x, y, r); }
static void S_circle(IB *b, float x, float y, float r) { tf(b, b->sub); path_circle(b->sub, x, y, r); }
static void A_ellipse(IB *b, float x, float y, float rx, float ry) { tf(b, b->add); path_ellipse(b->add, x, y, rx, ry, 0); }
static void poly_into(IB *b, Path *p, const float *xy, int n) {
    float area = 0;
    for (int i = 0; i < n; i++) { int j = (i + 1) % n; area += xy[2 * i] * xy[2 * j + 1] - xy[2 * j] * xy[2 * i + 1]; }
    tf(b, p);
    if (area >= 0) path_poly(p, xy, n);
    else {
        float tmp[64];
        for (int i = 0; i < n && i < 32; i++) { tmp[2 * i] = xy[2 * (n - 1 - i)]; tmp[2 * i + 1] = xy[2 * (n - 1 - i) + 1]; }
        path_poly(p, tmp, n);
    }
}
static void A_poly(IB *b, const float *xy, int n) { poly_into(b, b->add, xy, n); }
static void S_poly(IB *b, const float *xy, int n) { poly_into(b, b->sub, xy, n); }
static void A_line(IB *b, const float *xy, int n, float w) { tf(b, b->add); path_stroke(b->add, xy, n, w, 0); }
static void S_line(IB *b, const float *xy, int n, float w) { tf(b, b->sub); path_stroke(b->sub, xy, n, w, 0); }
static void A_arc(IB *b, float cx, float cy, float r, float a0, float a1, float w) { tf(b, b->add); path_arc_stroke(b->add, cx, cy, r, a0, a1, w); }
static void S_arc(IB *b, float cx, float cy, float r, float a0, float a1, float w) { tf(b, b->sub); path_arc_stroke(b->sub, cx, cy, r, a0, a1, w); }
static void A_ring(IB *b, float cx, float cy, float r, float w) { A_arc(b, cx, cy, r, 0, 2 * PI_F, w); }
/* Rounded-rect outline as a stroke, so inner glyphs aren't cut away. */
static void A_frame(IB *b, float x, float y, float w, float h, float r, float lw) {
    float pts[2 * 44];
    int n = 0;
    const float cx[4] = {x + w - r, x + w - r, x + r, x + r}, cy[4] = {y + r, y + h - r, y + h - r, y + r};
    for (int c = 0; c < 4; c++)
        for (int k = 0; k <= 10; k++) {
            float a = -PI_F * 0.5f + c * PI_F * 0.5f + k * PI_F * 0.05f;
            pts[2 * n] = cx[c] + cosf(a) * r; pts[2 * n + 1] = cy[c] + sinf(a) * r; n++;
        }
    tf(b, b->add);
    path_stroke(b->add, pts, n, lw, 1);
}

static void cloud(IB *b, float ox, float oy, float k) {
    A_circle(b, ox + 0.36f * k, oy + 0.56f * k, 0.17f * k);
    A_circle(b, ox + 0.55f * k, oy + 0.44f * k, 0.22f * k);
    A_circle(b, ox + 0.74f * k, oy + 0.58f * k, 0.15f * k);
    A_rrect(b, ox + 0.2f * k, oy + 0.55f * k, 0.68f * k, 0.18f * k, 0.09f * k);
}

static void build(int id, IB *b) {
    switch (id) {
    case IC_PLAY: { float t[] = {0.3f, 0.2f, 0.8f, 0.5f, 0.3f, 0.8f}; A_poly(b, t, 3); A_line(b, (float[]){0.3f, 0.2f, 0.8f, 0.5f, 0.3f, 0.8f, 0.3f, 0.2f}, 4, 0.08f); break; }
    case IC_PAUSE: A_rrect(b, 0.26f, 0.18f, 0.16f, 0.64f, 0.05f); A_rrect(b, 0.58f, 0.18f, 0.16f, 0.64f, 0.05f); break;
    case IC_PREV: { A_rrect(b, 0.2f, 0.22f, 0.1f, 0.56f, 0.03f); float t[] = {0.8f, 0.22f, 0.8f, 0.78f, 0.32f, 0.5f}; A_poly(b, t, 3); break; }
    case IC_NEXT: { A_rrect(b, 0.7f, 0.22f, 0.1f, 0.56f, 0.03f); float t[] = {0.2f, 0.22f, 0.68f, 0.5f, 0.2f, 0.78f}; A_poly(b, t, 3); break; }
    case IC_REW: { float t1[] = {0.5f, 0.24f, 0.5f, 0.76f, 0.12f, 0.5f}, t2[] = {0.88f, 0.24f, 0.88f, 0.76f, 0.5f, 0.5f}; A_poly(b, t1, 3); A_poly(b, t2, 3); break; }
    case IC_FFW: { float t1[] = {0.12f, 0.24f, 0.5f, 0.5f, 0.12f, 0.76f}, t2[] = {0.5f, 0.24f, 0.88f, 0.5f, 0.5f, 0.76f}; A_poly(b, t1, 3); A_poly(b, t2, 3); break; }
    case IC_PLUS: A_rrect(b, 0.44f, 0.16f, 0.12f, 0.68f, 0.06f); A_rrect(b, 0.16f, 0.44f, 0.68f, 0.12f, 0.06f); break;
    case IC_CHECK: A_line(b, (float[]){0.2f, 0.52f, 0.41f, 0.72f, 0.8f, 0.3f}, 3, 0.11f); break;
    case IC_INFO: A_ring(b, 0.5f, 0.5f, 0.38f, 0.08f); A_circle(b, 0.5f, 0.32f, 0.06f); A_rrect(b, 0.45f, 0.43f, 0.1f, 0.3f, 0.05f); break;
    case IC_SEARCH: A_ring(b, 0.43f, 0.43f, 0.24f, 0.1f); A_line(b, (float[]){0.62f, 0.62f, 0.82f, 0.82f}, 2, 0.12f); break;
    case IC_GEAR:
        A_circle(b, 0.5f, 0.5f, 0.29f);
        for (int i = 0; i < 8; i++) {
            float a = i * PI_F / 4, c = cosf(a), s = sinf(a), w = 0.085f;
            float px = -s * w, py = c * w;
            float p[] = {0.5f + c * 0.2f + px, 0.5f + s * 0.2f + py, 0.5f + c * 0.42f + px * 0.8f, 0.5f + s * 0.42f + py * 0.8f,
                         0.5f + c * 0.42f - px * 0.8f, 0.5f + s * 0.42f - py * 0.8f, 0.5f + c * 0.2f - px, 0.5f + s * 0.2f - py};
            A_poly(b, p, 4);
        }
        S_circle(b, 0.5f, 0.5f, 0.12f);
        break;
    case IC_HOME: { float p[] = {0.5f, 0.14f, 0.86f, 0.46f, 0.78f, 0.46f, 0.78f, 0.84f, 0.22f, 0.84f, 0.22f, 0.46f, 0.14f, 0.46f}; A_poly(b, p, 7); S_rrect(b, 0.42f, 0.58f, 0.16f, 0.26f, 0.03f); break; }
    case IC_FILM:
        A_rrect(b, 0.12f, 0.36f, 0.76f, 0.46f, 0.07f);
        { float p[] = {0.12f, 0.2f, 0.82f, 0.08f, 0.85f, 0.25f, 0.15f, 0.36f}; A_poly(b, p, 4); }
        for (int i = 0; i < 3; i++) { float p[] = {0.25f + i * 0.2f, 0.17f, 0.36f + i * 0.2f, 0.15f, 0.3f + i * 0.2f, 0.31f, 0.19f + i * 0.2f, 0.33f}; S_poly(b, p, 4); }
        break;
    case IC_TV: case IC_DISPLAY:
        A_frame(b, 0.115f, 0.195f, 0.77f, 0.49f, 0.05f, 0.07f);
        A_rrect(b, 0.3f, 0.8f, 0.4f, 0.07f, 0.035f);
        if (id == IC_TV) { float t[] = {0.43f, 0.35f, 0.6f, 0.44f, 0.43f, 0.53f}; A_poly(b, t, 3); }
        break;
    case IC_MUSIC:
        A_ellipse(b, 0.3f, 0.72f, 0.12f, 0.1f); A_ellipse(b, 0.72f, 0.64f, 0.12f, 0.1f);
        A_rrect(b, 0.36f, 0.22f, 0.07f, 0.52f, 0.02f); A_rrect(b, 0.78f, 0.14f, 0.07f, 0.52f, 0.02f);
        { float p[] = {0.36f, 0.22f, 0.85f, 0.12f, 0.85f, 0.26f, 0.36f, 0.36f}; A_poly(b, p, 4); }
        break;
    case IC_PHOTO:
        A_frame(b, 0.135f, 0.215f, 0.73f, 0.57f, 0.06f, 0.07f);
        { float m[] = {0.2f, 0.72f, 0.42f, 0.42f, 0.56f, 0.6f, 0.64f, 0.5f, 0.8f, 0.72f}; A_poly(b, m, 5); }
        A_circle(b, 0.66f, 0.36f, 0.06f);
        break;
    case IC_BACK: A_line(b, (float[]){0.48f, 0.24f, 0.22f, 0.5f, 0.48f, 0.76f}, 3, 0.1f); A_line(b, (float[]){0.24f, 0.5f, 0.8f, 0.5f}, 2, 0.1f); break;
    case IC_CHEV_L: A_line(b, (float[]){0.62f, 0.2f, 0.34f, 0.5f, 0.62f, 0.8f}, 3, 0.11f); break;
    case IC_CHEV_R: A_line(b, (float[]){0.38f, 0.2f, 0.66f, 0.5f, 0.38f, 0.8f}, 3, 0.11f); break;
    case IC_CHEV_D: A_line(b, (float[]){0.2f, 0.38f, 0.5f, 0.66f, 0.8f, 0.38f}, 3, 0.11f); break;
    case IC_CHEV_U: A_line(b, (float[]){0.2f, 0.62f, 0.5f, 0.34f, 0.8f, 0.62f}, 3, 0.11f); break;
    case IC_STAR: case IC_SPARKLE: {
        float p[20];
        int n = id == IC_STAR ? 5 : 4;
        float ri = id == IC_STAR ? 0.18f : 0.1f;
        for (int i = 0; i < 2 * n; i++) {
            float a = -PI_F / 2 + i * PI_F / n, r = (i & 1) ? ri : 0.42f;
            p[2 * i] = 0.5f + cosf(a) * r; p[2 * i + 1] = 0.53f + sinf(a) * r;
        }
        A_poly(b, p, 2 * n);
        break;
    }
    case IC_HEART: {
        A_circle(b, 0.34f, 0.38f, 0.18f); A_circle(b, 0.66f, 0.38f, 0.18f);
        float p[] = {0.17f, 0.44f, 0.5f, 0.42f, 0.83f, 0.44f, 0.5f, 0.84f};
        A_poly(b, p, 4);
        break;
    }
    case IC_WIFI:
        A_arc(b, 0.5f, 0.78f, 0.6f, -PI_F * 0.75f, -PI_F * 0.25f, 0.09f);
        A_arc(b, 0.5f, 0.78f, 0.4f, -PI_F * 0.75f, -PI_F * 0.25f, 0.09f);
        A_arc(b, 0.5f, 0.78f, 0.2f, -PI_F * 0.75f, -PI_F * 0.25f, 0.09f);
        A_circle(b, 0.5f, 0.78f, 0.07f);
        break;
    case IC_VOLUME: {
        float p[] = {0.12f, 0.38f, 0.28f, 0.38f, 0.48f, 0.18f, 0.48f, 0.82f, 0.28f, 0.62f, 0.12f, 0.62f};
        A_poly(b, p, 6);
        A_arc(b, 0.5f, 0.5f, 0.16f, -PI_F * 0.3f, PI_F * 0.3f, 0.07f);
        A_arc(b, 0.5f, 0.5f, 0.32f, -PI_F * 0.3f, PI_F * 0.3f, 0.07f);
        break;
    }
    case IC_SHUFFLE:
        A_line(b, (float[]){0.14f, 0.3f, 0.32f, 0.3f, 0.6f, 0.7f, 0.8f, 0.7f}, 4, 0.08f);
        A_line(b, (float[]){0.14f, 0.7f, 0.32f, 0.7f, 0.6f, 0.3f, 0.8f, 0.3f}, 4, 0.08f);
        { float t1[] = {0.78f, 0.18f, 0.92f, 0.3f, 0.78f, 0.42f}, t2[] = {0.78f, 0.58f, 0.92f, 0.7f, 0.78f, 0.82f}; A_poly(b, t1, 3); A_poly(b, t2, 3); }
        break;
    case IC_REPEAT:
        A_line(b, (float[]){0.2f, 0.56f, 0.2f, 0.32f, 0.76f, 0.32f}, 3, 0.08f);
        A_line(b, (float[]){0.8f, 0.44f, 0.8f, 0.68f, 0.24f, 0.68f}, 3, 0.08f);
        { float t1[] = {0.7f, 0.2f, 0.84f, 0.32f, 0.7f, 0.44f}, t2[] = {0.3f, 0.56f, 0.16f, 0.68f, 0.3f, 0.8f}; A_poly(b, t1, 3); A_poly(b, t2, 3); }
        break;
    case IC_USER:
        A_circle(b, 0.5f, 0.34f, 0.17f);
        A_ellipse(b, 0.5f, 0.9f, 0.32f, 0.32f);
        S_rrect(b, 0.0f, 0.86f, 1.0f, 0.2f, 0);
        break;
    case IC_GRID:
        for (int i = 0; i < 4; i++) A_rrect(b, 0.16f + (i % 2) * 0.36f, 0.16f + (i / 2) * 0.36f, 0.3f, 0.3f, 0.06f);
        break;
    case IC_WEATHER:
        A_circle(b, 0.38f, 0.36f, 0.16f);
        for (int i = 0; i < 6; i++) { float a = PI_F * 0.85f + i * PI_F / 5; A_line(b, (float[]){0.38f + cosf(a) * 0.22f, 0.36f + sinf(a) * 0.22f, 0.38f + cosf(a) * 0.29f, 0.36f + sinf(a) * 0.29f}, 2, 0.05f); }
        cloud(b, 0.08f, 0.12f, 0.9f);
        break;
    case IC_GAMEPAD:
        A_rrect(b, 0.08f, 0.28f, 0.84f, 0.46f, 0.22f);
        S_rrect(b, 0.2f, 0.47f, 0.2f, 0.07f, 0.02f); S_rrect(b, 0.265f, 0.405f, 0.07f, 0.2f, 0.02f);
        S_circle(b, 0.66f, 0.45f, 0.045f); S_circle(b, 0.76f, 0.55f, 0.045f);
        break;
    case IC_FITNESS:
        A_rrect(b, 0.25f, 0.45f, 0.5f, 0.1f, 0.03f);
        A_rrect(b, 0.13f, 0.28f, 0.12f, 0.44f, 0.05f); A_rrect(b, 0.75f, 0.28f, 0.12f, 0.44f, 0.05f);
        A_rrect(b, 0.05f, 0.37f, 0.08f, 0.26f, 0.03f); A_rrect(b, 0.87f, 0.37f, 0.08f, 0.26f, 0.03f);
        break;
    case IC_NEWS:
        A_rrect(b, 0.14f, 0.16f, 0.72f, 0.68f, 0.08f);
        S_rrect(b, 0.24f, 0.27f, 0.24f, 0.2f, 0.02f);
        S_rrect(b, 0.54f, 0.28f, 0.22f, 0.05f, 0.02f); S_rrect(b, 0.54f, 0.4f, 0.22f, 0.05f, 0.02f);
        S_rrect(b, 0.24f, 0.55f, 0.52f, 0.05f, 0.02f); S_rrect(b, 0.24f, 0.66f, 0.4f, 0.05f, 0.02f);
        break;
    case IC_MIC:
        A_rrect(b, 0.38f, 0.12f, 0.24f, 0.44f, 0.12f);
        A_arc(b, 0.5f, 0.42f, 0.24f, 0.05f, PI_F - 0.05f, 0.07f);
        A_rrect(b, 0.465f, 0.66f, 0.07f, 0.16f, 0.02f); A_rrect(b, 0.34f, 0.8f, 0.32f, 0.07f, 0.035f);
        break;
    case IC_KIDS:
        A_ellipse(b, 0.5f, 0.38f, 0.24f, 0.28f);
        { float k[] = {0.46f, 0.68f, 0.54f, 0.68f, 0.5f, 0.62f}; A_poly(b, k, 3); }
        A_line(b, (float[]){0.5f, 0.68f, 0.44f, 0.78f, 0.54f, 0.88f}, 3, 0.04f);
        { tf(b, b->sub); path_ellipse(b->sub, 0.42f, 0.28f, 0.05f, 0.09f, -0.4f); }
        break;
    case IC_ACCESS:
        A_ring(b, 0.5f, 0.5f, 0.38f, 0.07f); A_circle(b, 0.5f, 0.3f, 0.06f);
        A_line(b, (float[]){0.3f, 0.42f, 0.7f, 0.42f}, 2, 0.07f);
        A_line(b, (float[]){0.5f, 0.42f, 0.5f, 0.56f, 0.38f, 0.74f}, 3, 0.07f);
        A_line(b, (float[]){0.5f, 0.56f, 0.62f, 0.74f}, 2, 0.07f);
        break;
    case IC_BELL:
        A_rrect(b, 0.3f, 0.22f, 0.4f, 0.46f, 0.2f); A_rrect(b, 0.3f, 0.45f, 0.4f, 0.23f, 0.0f);
        A_rrect(b, 0.18f, 0.64f, 0.64f, 0.08f, 0.04f); A_circle(b, 0.5f, 0.8f, 0.07f); A_circle(b, 0.5f, 0.18f, 0.05f);
        break;
    case IC_POWER: A_arc(b, 0.5f, 0.54f, 0.3f, -PI_F * 0.32f, PI_F * 1.32f, 0.09f); A_line(b, (float[]){0.5f, 0.14f, 0.5f, 0.48f}, 2, 0.09f); break;
    case IC_SLIDESHOW:
        A_rrect(b, 0.1f, 0.24f, 0.8f, 0.56f, 0.08f); S_poly(b, (float[]){0.42f, 0.38f, 0.64f, 0.52f, 0.42f, 0.66f}, 3);
        A_rrect(b, 0.2f, 0.14f, 0.6f, 0.06f, 0.03f);
        break;
    case IC_DELETE: {
        float p[] = {0.32f, 0.22f, 0.9f, 0.22f, 0.9f, 0.78f, 0.32f, 0.78f, 0.08f, 0.5f};
        A_poly(b, p, 5);
        S_line(b, (float[]){0.48f, 0.38f, 0.72f, 0.62f}, 2, 0.07f); S_line(b, (float[]){0.72f, 0.38f, 0.48f, 0.62f}, 2, 0.07f);
        break;
    }
    case IC_SPACE: A_line(b, (float[]){0.12f, 0.42f, 0.12f, 0.66f, 0.88f, 0.66f, 0.88f, 0.42f}, 4, 0.08f); break;
    case IC_GLOBE:
        A_ring(b, 0.5f, 0.5f, 0.36f, 0.06f);
        { tf(b, b->add); Path *p = b->add; for (int i = 0; i < 2; i++) { float rx = i ? 0.16f : 0.001f; int n = 40; float pts[82]; for (int k = 0; k <= n; k++) { float a = 2 * PI_F * k / n; pts[2 * k] = 0.5f + cosf(a) * rx; pts[2 * k + 1] = 0.5f + sinf(a) * 0.36f; } path_stroke(p, pts, n + 1, 0.05f, 0); } }
        A_line(b, (float[]){0.16f, 0.5f, 0.84f, 0.5f}, 2, 0.05f);
        A_line(b, (float[]){0.22f, 0.32f, 0.78f, 0.32f}, 2, 0.04f); A_line(b, (float[]){0.22f, 0.68f, 0.78f, 0.68f}, 2, 0.04f);
        break;
    case IC_CC:
        A_rrect(b, 0.08f, 0.2f, 0.84f, 0.6f, 0.1f);
        S_arc(b, 0.36f, 0.5f, 0.12f, PI_F * 0.3f, PI_F * 1.7f, 0.07f);
        S_arc(b, 0.66f, 0.5f, 0.12f, PI_F * 0.3f, PI_F * 1.7f, 0.07f);
        break;
    case IC_SUN:
        A_circle(b, 0.5f, 0.5f, 0.2f);
        for (int i = 0; i < 8; i++) { float a = i * PI_F / 4; A_line(b, (float[]){0.5f + cosf(a) * 0.3f, 0.5f + sinf(a) * 0.3f, 0.5f + cosf(a) * 0.4f, 0.5f + sinf(a) * 0.4f}, 2, 0.06f); }
        break;
    case IC_CLOUD: cloud(b, 0, 0, 1); break;
    case IC_RAIN:
        cloud(b, 0, -0.1f, 1);
        for (int i = 0; i < 3; i++) A_line(b, (float[]){0.34f + i * 0.16f, 0.74f, 0.29f + i * 0.16f, 0.88f}, 2, 0.05f);
        break;
    case IC_SNOW:
        cloud(b, 0, -0.1f, 1);
        for (int i = 0; i < 3; i++) A_circle(b, 0.34f + i * 0.16f, 0.82f - (i & 1) * 0.04f, 0.04f);
        break;
    case IC_BOLT: { float p[] = {0.56f, 0.1f, 0.24f, 0.56f, 0.46f, 0.56f, 0.4f, 0.9f, 0.76f, 0.42f, 0.54f, 0.42f}; A_poly(b, p, 6); break; }
    case IC_PALETTE:
        A_circle(b, 0.5f, 0.5f, 0.38f);
        S_circle(b, 0.34f, 0.36f, 0.06f); S_circle(b, 0.52f, 0.28f, 0.06f); S_circle(b, 0.68f, 0.38f, 0.06f); S_circle(b, 0.62f, 0.66f, 0.09f);
        break;
    case IC_LIST:
        for (int i = 0; i < 3; i++) { A_circle(b, 0.2f, 0.28f + i * 0.22f, 0.05f); A_rrect(b, 0.32f, 0.245f + i * 0.22f, 0.5f, 0.07f, 0.035f); }
        break;
    case IC_LIVE:
        A_circle(b, 0.5f, 0.5f, 0.12f);
        A_arc(b, 0.5f, 0.5f, 0.25f, -PI_F * 0.3f, PI_F * 0.3f, 0.07f); A_arc(b, 0.5f, 0.5f, 0.25f, PI_F * 0.7f, PI_F * 1.3f, 0.07f);
        A_arc(b, 0.5f, 0.5f, 0.4f, -PI_F * 0.3f, PI_F * 0.3f, 0.07f); A_arc(b, 0.5f, 0.5f, 0.4f, PI_F * 0.7f, PI_F * 1.3f, 0.07f);
        break;
    }
}

static Mask *make_mask(int id, int size, float ox, float oy) {
    Path add, sub;
    path_init(&add);
    path_init(&sub);
    IB b = {&add, &sub, ox, oy, (float)size};
    build(id, &b);
    Mask *m = path_to_mask_fixed(&add, size, size);
    if (sub.nseg) {
        Mask *s = path_to_mask_fixed(&sub, size, size);
        for (int i = 0; i < size * size; i++) m->a[i] = (uint8_t)MAXI(0, (int)m->a[i] - (int)s->a[i]);
        mask_free(s);
    }
    path_free(&add);
    path_free(&sub);
    return m;
}

typedef struct { int id, size; Mask *m; } IconEntry;
static IconEntry g_icons[512];
static int g_nicons;
static pthread_mutex_t g_icon_lock = PTHREAD_MUTEX_INITIALIZER;

const Mask *icon_mask(int id, int size) {
    pthread_mutex_lock(&g_icon_lock);
    for (int i = 0; i < g_nicons; i++)
        if (g_icons[i].id == id && g_icons[i].size == size) { Mask *m = g_icons[i].m; pthread_mutex_unlock(&g_icon_lock); return m; }
    pthread_mutex_unlock(&g_icon_lock);
    Mask *m = make_mask(id, size, 0, 0);
    pthread_mutex_lock(&g_icon_lock);
    if (g_nicons < 512) g_icons[g_nicons++] = (IconEntry){id, size, m};
    pthread_mutex_unlock(&g_icon_lock);
    return m;
}

void g_icon(Gfx *g, int id, float x, float y, float size, uint32_t c) {
    int s = (int)(size + 0.5f);
    if (s < 4) return;
    const Mask *m = icon_mask(id, s);
    g_mask(g, m, roundf(x), roundf(y), (float)s, (float)s, c);
}

void icon_blit(Surface *s, int id, float x, float y, float size, uint32_t c) {
    int n = (int)ceilf(size) + 2;
    int ix = (int)floorf(x), iy = (int)floorf(y);
    Path add, sub;
    Mask *m;
    path_init(&add);
    path_init(&sub);
    IB b = {&add, &sub, x - ix, y - iy, size};
    build(id, &b);
    m = path_to_mask_fixed(&add, n, n);
    if (sub.nseg) {
        Mask *sm = path_to_mask_fixed(&sub, n, n);
        for (int i = 0; i < n * n; i++) m->a[i] = (uint8_t)MAXI(0, (int)m->a[i] - (int)sm->a[i]);
        mask_free(sm);
    }
    path_free(&add);
    path_free(&sub);
    for (int yy = 0; yy < n; yy++) {
        int py = iy + yy;
        if (py < 0 || py >= s->h) continue;
        for (int xx = 0; xx < n; xx++) {
            int px = ix + xx;
            if (px < 0 || px >= s->w) continue;
            uint32_t a = m->a[yy * n + xx];
            if (a) s->px[(size_t)py * s->stride + px] = blend(s->px[(size_t)py * s->stride + px], mul8(c, a));
        }
    }
    mask_free(m);
}

/* ---------------- App tiles ---------------- */
Texture *gen_app_tile(int idx) {
    const AppInfo *a = &g_apps[idx];
    const int S = 256;
    Texture *t = tex_new(S, S);
    Surface s = tex_surface(t);
    Gfx *g = gfx_new_immediate(s);
    g_rect_grad(g, 0, 0, S, S, 0, a->c0 | 0xFF000000u, a->c1 | 0xFF000000u, 0, 0, S, S);
    /* Soft light from the top-left and a glossy sheen. */
    g_glow(g, 40, 20, 200, rgba(255, 255, 255, 70));
    g_rect_vgrad(g, 0, 0, S, S * 0.5f, 0, rgba(255, 255, 255, 40), rgba(255, 255, 255, 0));
    /* Icon with a subtle drop shadow. */
    icon_blit(&s, a->icon, 64, 70, 128, rgba(0, 0, 0, 50));
    icon_blit(&s, a->icon, 64, 64, 128, WHITE);
    gfx_free(g);
    tex_mark_opaque(t);
    tex_build_mips(t, 32);
    return t;
}

/* ---------------- Avatars ---------------- */
Texture *gen_avatar(int idx) {
    const Profile *p = &g_profiles[idx];
    const int S = 256;
    Texture *t = tex_new(S, S);
    Surface s = tex_surface(t);
    Gfx *g = gfx_new_immediate(s);
    g_rect_grad(g, 0, 0, S, S, 0, p->c0 | 0xFF000000u, p->c1 | 0xFF000000u, 0, 0, S, S);
    g_glow(g, 60, 40, 180, rgba(255, 255, 255, 80));
    uint32_t ink = rgba(40, 30, 60, 255);
    switch (p->style) {
    case 0: /* friendly blob */
        g_shadow(g, 48, 70, 160, 170, 80, 24, rgba(0, 0, 0, 50));
        g_rect(g, 48, 60, 160, 170, 80, rgba(255, 250, 245, 255));
        g_circle(g, 100, 130, 13, ink); g_circle(g, 156, 130, 13, ink);
        g_circle(g, 104, 126, 4, WHITE); g_circle(g, 160, 126, 4, WHITE);
        g_circle(g, 82, 160, 12, rgba(255, 120, 140, 120)); g_circle(g, 174, 160, 12, rgba(255, 120, 140, 120));
        { Path pa; path_init(&pa); path_arc_stroke(&pa, 128, 150, 24, 0.35f, PI_F - 0.35f, 7); path_fill_color(&s, &pa, ink); path_free(&pa); }
        break;
    case 1: /* robot */
        g_line(g, 128, 50, 128, 80, 6, rgba(255, 255, 255, 230)); g_circle(g, 128, 46, 11, rgba(255, 220, 90, 255));
        g_shadow(g, 56, 82, 144, 130, 36, 24, rgba(0, 0, 0, 60));
        g_rect(g, 56, 78, 144, 130, 36, rgba(245, 250, 255, 255));
        g_rect(g, 72, 104, 112, 58, 24, rgba(30, 40, 70, 255));
        g_rect(g, 90, 120, 22, 26, 10, rgba(100, 240, 255, 255)); g_rect(g, 144, 120, 22, 26, 10, rgba(100, 240, 255, 255));
        g_rect(g, 104, 180, 48, 8, 4, rgba(30, 40, 70, 200));
        g_rect(g, 40, 120, 18, 44, 9, rgba(220, 230, 245, 255)); g_rect(g, 198, 120, 18, 44, 9, rgba(220, 230, 245, 255));
        break;
    case 2: { /* cat */
        Path pa; path_init(&pa);
        float e1[] = {60, 120, 74, 44, 120, 92}, e2[] = {196, 120, 182, 44, 136, 92};
        path_poly(&pa, e1, 3); path_poly(&pa, e2, 3);
        path_fill_color(&s, &pa, rgba(255, 255, 255, 255)); path_free(&pa);
        g_shadow(g, 52, 80, 152, 140, 70, 20, rgba(0, 0, 0, 50));
        g_rect(g, 52, 76, 152, 140, 70, rgba(255, 255, 255, 255));
        g_circle(g, 100, 136, 11, ink); g_circle(g, 156, 136, 11, ink);
        g_circle(g, 103, 132, 3.5f, WHITE); g_circle(g, 159, 132, 3.5f, WHITE);
        { float n[] = {120, 158, 136, 158, 128, 168}; g_poly(g, n, 3, rgba(255, 120, 150, 255)); }
        g_line(g, 70, 160, 40, 154, 3, rgba(120, 100, 120, 160)); g_line(g, 70, 168, 42, 172, 3, rgba(120, 100, 120, 160));
        g_line(g, 186, 160, 216, 154, 3, rgba(120, 100, 120, 160)); g_line(g, 186, 168, 214, 172, 3, rgba(120, 100, 120, 160));
        break;
    }
    default: /* astronaut */
        g_shadow(g, 52, 56, 152, 160, 76, 24, rgba(0, 0, 0, 60));
        g_rect(g, 52, 52, 152, 160, 76, rgba(250, 250, 255, 255));
        g_rect_grad(g, 72, 84, 112, 88, 44, rgba(40, 30, 90, 255), rgba(120, 60, 200, 255), 72, 84, 184, 172);
        g_glow(g, 104, 108, 30, rgba(255, 255, 255, 120));
        g_rect(g, 92, 102, 26, 12, 6, rgba(255, 255, 255, 150));
        g_circle(g, 160, 150, 3, WHITE); g_circle(g, 150, 160, 2, WHITE); g_circle(g, 120, 154, 2, rgba(255, 255, 255, 180));
        break;
    }
    gfx_free(g);
    tex_mark_opaque(t);
    tex_build_mips(t, 32);
    return t;
}
