/* KEEL OS — anti-aliased vector path rasterizer (signed-area accumulation). */
#pragma once
#include "gfx.h"

typedef struct Path {
    float *seg;          /* x0,y0,x1,y1 line segments after flattening */
    int nseg, cap;
    float cx, cy, sx, sy; /* current point, contour start */
    float m[6];          /* affine transform: x' = m0 x + m1 y + m2, y' = m3 x + m4 y + m5 */
    int open;
} Path;

enum { PAINT_SOLID, PAINT_LINEAR, PAINT_RADIAL };
typedef struct {
    int type;
    uint32_t c0, c1;
    float x0, y0, x1, y1, r;
} Paint;

void path_init(Path *p);
void path_free(Path *p);
void path_reset(Path *p);
void path_transform(Path *p, float a, float b, float c, float d, float e, float f);
void path_identity(Path *p);
void path_move(Path *p, float x, float y);
void path_line(Path *p, float x, float y);
void path_quad(Path *p, float x1, float y1, float x2, float y2);
void path_cubic(Path *p, float x1, float y1, float x2, float y2, float x3, float y3);
void path_close(Path *p);
void path_rect(Path *p, float x, float y, float w, float h);
void path_rrect(Path *p, float x, float y, float w, float h, float r);
void path_ellipse(Path *p, float cx, float cy, float rx, float ry, float rot);
void path_circle(Path *p, float cx, float cy, float r);
void path_poly(Path *p, const float *xy, int n);
/* Thick polyline as a union of quads and round joins (separate path from fills). */
void path_stroke(Path *p, const float *xy, int n, float width, int closed);
void path_arc_stroke(Path *p, float cx, float cy, float r, float a0, float a1, float width);

void path_fill(Surface *s, const Path *p, const Paint *paint);
static inline void path_fill_color(Surface *s, const Path *p, uint32_t c) {
    Paint pt = {PAINT_SOLID, c, c, 0, 0, 0, 0, 0};
    path_fill(s, p, &pt);
}
/* Rasterize into a new A8 mask; returns origin offset of the mask in path space. */
Mask *path_to_mask(const Path *p, int *ox, int *oy, float gamma);
Mask *path_to_mask_fixed(const Path *p, int w, int h);
void mask_free(Mask *m);
