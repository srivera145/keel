/* KEEL OS — shared visual components. */
#pragma once
#include "app.h"
#include "assets.h"
#include "font.h"
#include "icons.h"

#define SAFE_X 96.f

/* Frosted glass panel: blur of what's behind + bright tint + edge light. */
void ui_glass(Gfx *g, float x, float y, float w, float h, float r, float strength);
/* Dark glass for overlays on bright video. */
void ui_glass_dark(Gfx *g, float x, float y, float w, float h, float r, float strength);
/* Full-screen backdrop image with cinematic scrims. */
void ui_backdrop(Gfx *g, const Texture *t, float alpha, int heavy_scrim);
void ui_wallpaper(Gfx *g, float alpha);
/* Top navigation (tab bar, logo, clock, avatar). */
void ui_nav(Gfx *g, int tab, float alpha);
int ui_nav_input(int key);
/* Pill button. f = focus amount 0..1. Returns width. */
float ui_button(Gfx *g, float x, float y, float h, int icon, const char *label, float f, int primary);
float ui_button_width(float h, int icon, const char *label);
/* Focus ring + lift for cards. */
void ui_card_shadow(Gfx *g, float x, float y, float w, float h, float r, float f);
void ui_focus_ring(Gfx *g, float x, float y, float w, float h, float r, float f);
/* Scale a rect around its center. */
void ui_scale(float *x, float *y, float *w, float *h, float s);
/* Image with a loading shimmer until ready. */
void ui_tex(Gfx *g, const Texture *t, float x, float y, float w, float h, float r, float alpha);
void ui_section(Gfx *g, float x, float y, const char *title, float alpha);
void ui_progress(Gfx *g, float x, float y, float w, float h, float p, uint32_t fill);
void ui_stars(Gfx *g, float x, float y, float size, float score);
void ui_badge(Gfx *g, float x, float y, const char *text, float alpha);
float ui_badge_width(const char *text);
void ui_toggle(Gfx *g, float x, float y, int on, float f);
void ui_spinner(Gfx *g, float cx, float cy, float r, float alpha);
void ui_fps_overlay(Gfx *g);
void ui_toast_draw(Gfx *g);
void ui_logo(Gfx *g, float x, float y, float size, float alpha);

/* Animated focus values: smoothly approach 1 for the focused index. */
typedef struct { float v[64]; } FocusAnim;
void focus_update(FocusAnim *a, int n, int focused, float dt);
/* Smooth scroll toward target. */
float scroll_to(float cur, float target, float dt);
