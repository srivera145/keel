/* KEEL OS — software compositor: surfaces, display list, primitives. */
#pragma once
#include "core.h"

typedef struct { int x0, y0, x1, y1; } IRect;

typedef struct Texture {
    int w, h;
    uint32_t *px;          /* premultiplied ARGB, stride == w */
    struct Texture *mip;   /* half-size chain (optional) */
    int opaque;
    int ytop;              /* rows above this are fully transparent */
    volatile int ready;    /* set when background generation finished */
} Texture;

typedef struct Mask {
    int w, h;
    uint8_t *a;
} Mask;

typedef struct Surface {
    uint32_t *px;
    int w, h, stride;
} Surface;

Texture *tex_new(int w, int h);
void tex_free(Texture *t);
void tex_build_mips(Texture *t, int min_size);
Surface tex_surface(Texture *t);
void tex_clear(Texture *t, uint32_t c);
/* Box-filtered downscale into a new texture. */
Texture *tex_downscale(const Texture *src, int w, int h);
/* Separable blur in place (radius in pixels, 3 box passes). */
void tex_blur(Texture *t, int radius);
void tex_mark_opaque(Texture *t);

/* ---- Glyph positioned run (filled by font module) ---- */
typedef struct { const Mask *m; int x, y; } GlyphPos;

/* ---- Drawing context: records a display list or draws immediately ---- */
typedef struct Gfx Gfx;

Gfx *gfx_new_recorder(void);
Gfx *gfx_new_immediate(Surface target);
void gfx_free(Gfx *g);
void gfx_set_target(Gfx *g, Surface target);
void gfx_begin(Gfx *g);                 /* reset recorder for a new frame */
void gfx_execute(Gfx *g, Surface *fb);  /* run the display list on the pool */
int gfx_cmd_count(Gfx *g);
void *gfx_alloc(Gfx *g, size_t n);      /* per-frame arena memory */

/* State: translation, opacity, clip. Push/pop pairs. */
void g_push(Gfx *g);
void g_pop(Gfx *g);
void g_translate(Gfx *g, float dx, float dy);
void g_opacity(Gfx *g, float a);        /* multiplies */
void g_clip(Gfx *g, float x, float y, float w, float h); /* intersects */
float g_get_opacity(Gfx *g);

/* Primitives */
void g_rect(Gfx *g, float x, float y, float w, float h, float r, uint32_t c);
void g_rect_vgrad(Gfx *g, float x, float y, float w, float h, float r, uint32_t top, uint32_t bottom);
void g_rect_hgrad(Gfx *g, float x, float y, float w, float h, float r, uint32_t left, uint32_t right);
void g_rect_grad(Gfx *g, float x, float y, float w, float h, float r, uint32_t c0, uint32_t c1,
                 float gx0, float gy0, float gx1, float gy1);
void g_shadow(Gfx *g, float x, float y, float w, float h, float r, float blur, uint32_t c);
void g_stroke(Gfx *g, float x, float y, float w, float h, float r, float width, uint32_t c);
void g_circle(Gfx *g, float cx, float cy, float rad, uint32_t c);
void g_glow(Gfx *g, float cx, float cy, float rad, uint32_t c); /* soft radial blob */
void g_line(Gfx *g, float x0, float y0, float x1, float y1, float width, uint32_t c);
void g_poly(Gfx *g, const float *xy, int n, uint32_t c); /* convex, n<=8 */
void g_image(Gfx *g, const Texture *t, float x, float y, float w, float h, float r, float alpha);
void g_image_src(Gfx *g, const Texture *t, float sx, float sy, float sw, float sh,
                 float x, float y, float w, float h, float r, float alpha);
void g_image_cover(Gfx *g, const Texture *t, float x, float y, float w, float h, float r, float alpha);
void g_tile(Gfx *g, const Texture *t, float x, float y, float w, float h, float ox, float oy, float alpha);
void g_mask(Gfx *g, const Mask *m, float x, float y, float w, float h, uint32_t c);
void g_glyphs(Gfx *g, const GlyphPos *gp, int n, uint32_t c);
/* Frosted backdrop: blur what's already drawn under the rect, then tint. */
void g_backdrop(Gfx *g, float x, float y, float w, float h, float r, int radius,
                uint32_t tint, float saturation);
/* Layers: draw content between begin/end, then it is composited back
 * with rounded anti-aliased corners and opacity. */
void g_layer_begin(Gfx *g, float x, float y, float w, float h);
void g_layer_end(Gfx *g, float radius, float alpha);
/* Low-level direct fill of a whole surface (immediate). */
void surf_fill(Surface *s, uint32_t c);

extern int g_stat_blur_regions;
extern int g_profile;
void gfx_profile_dump(void);
