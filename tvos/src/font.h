/* KEEL OS — font engine (TrueType glyf + CFF outlines), glyph cache, text layout. */
#pragma once
#include "gfx.h"

enum {
    FONT_THIN, FONT_LIGHT, FONT_REGULAR, FONT_MEDIUM, FONT_SEMIBOLD, FONT_BOLD,
    FONT_DISPLAY, FONT_BLACK, FONT_COUNT
};
enum { ALIGN_LEFT = 0, ALIGN_CENTER = 1, ALIGN_RIGHT = 2 };

int fonts_init(void);
const char *font_path(int font);
float font_cap(int font, float size);     /* cap height in px */
float font_ascent(int font, float size);
/* Width of a string in px. tracking is extra letter spacing in px. */
float text_width(int font, float size, const char *s, float tracking);
/* Draw text; y is the top of capital letters. Returns advance width. */
float g_text(Gfx *g, int font, float size, float x, float y, const char *s, uint32_t c, int align, float tracking);
/* Single line, truncated with an ellipsis to fit max_w. */
float g_text_fit(Gfx *g, int font, float size, float x, float y, const char *s, uint32_t c, float max_w, int align);
/* Word-wrapped paragraph; returns number of lines drawn. */
int g_text_wrap(Gfx *g, int font, float size, float x, float y, float max_w, float line_h, int max_lines,
                const char *s, uint32_t c);
/* Text with a soft drop shadow for legibility over imagery. */
float g_text_shadow(Gfx *g, int font, float size, float x, float y, const char *s, uint32_t c, int align, float tracking);
int font_glyph_count(int font);
