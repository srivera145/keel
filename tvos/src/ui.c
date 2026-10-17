/* KEEL OS — shared visual components. */
#include "ui.h"
#include "raster.h"

void focus_update(FocusAnim *a, int n, int focused, float dt) {
    float rate = app.set.reduce_motion ? 40.f : 13.f;
    for (int i = 0; i < n && i < 64; i++) a->v[i] = approach(a->v[i], i == focused ? 1.f : 0.f, dt, rate);
}
float scroll_to(float cur, float target, float dt) {
    float s = approach(cur, target, dt, app.set.reduce_motion ? 40.f : 9.f);
    return fabsf(s - target) < 0.3f ? target : s;
}
void ui_scale(float *x, float *y, float *w, float *h, float s) {
    float nw = *w * s, nh = *h * s;
    *x -= (nw - *w) * 0.5f; *y -= (nh - *h) * 0.5f;
    *w = nw; *h = nh;
}

void ui_glass(Gfx *g, float x, float y, float w, float h, float r, float strength) {
    int radius = 5 + app.set.glass * 2;
    g_shadow(g, x, y + 12, w, h, r, 44, rgba(10, 0, 30, (int)(55 * strength)));
    g_backdrop(g, x, y, w, h, r, radius, rgba(255, 255, 255, (int)(34 + 26 * strength)), 1.55f);
    g_rect_vgrad(g, x, y, w, fminf(h, 120.f), r, rgba(255, 255, 255, (int)(30 * strength)), rgba(255, 255, 255, 0));
    g_stroke(g, x, y, w, h, r, 1.5f, rgba(255, 255, 255, (int)(85 * strength)));
}
void ui_glass_dark(Gfx *g, float x, float y, float w, float h, float r, float strength) {
    int radius = 5 + app.set.glass * 2;
    g_shadow(g, x, y + 10, w, h, r, 40, rgba(0, 0, 0, (int)(60 * strength)));
    g_backdrop(g, x, y, w, h, r, radius, rgba(16, 12, 30, (int)(90 * strength)), 1.4f);
    g_stroke(g, x, y, w, h, r, 1.5f, rgba(255, 255, 255, (int)(55 * strength)));
}

void ui_backdrop(Gfx *g, const Texture *t, float alpha, int heavy) {
    if (t && t->ready) g_image(g, t, 0, 0, SCREEN_W, SCREEN_H, 0, alpha);
    else if (g_wallpaper) g_image(g, g_wallpaper, 0, 0, SCREEN_W, SCREEN_H, 0, alpha);
    float a = alpha;
    g_rect_hgrad(g, 0, 0, SCREEN_W * 0.62f, SCREEN_H, 0, rgba(8, 4, 24, (int)((heavy ? 150 : 115) * a)), rgba(8, 4, 24, 0));
    g_rect_vgrad(g, 0, SCREEN_H * 0.45f, SCREEN_W, SCREEN_H * 0.55f, 0, rgba(8, 4, 24, 0), rgba(8, 4, 24, (int)((heavy ? 200 : 150) * a)));
    g_rect_vgrad(g, 0, 0, SCREEN_W, 200, 0, rgba(8, 4, 24, (int)(70 * a)), rgba(8, 4, 24, 0));
}

void ui_wallpaper(Gfx *g, float alpha) {
    if (g_wallpaper && g_wallpaper->ready) g_image(g, g_wallpaper, 0, 0, SCREEN_W, SCREEN_H, 0, alpha);
    else g_rect_grad(g, 0, 0, SCREEN_W, SCREEN_H, 0, HEX(0x6A3FA0), HEX(0xFF8A65), 0, 0, SCREEN_W, SCREEN_H);
}

void ui_logo(Gfx *g, float x, float y, float size, float alpha) {
    g_push(g);
    g_opacity(g, alpha);
    float s = size;
    g_shadow(g, x, y + s * 0.08f, s, s, s * 0.3f, s * 0.3f, rgba(0, 0, 0, 60));
    g_rect_grad(g, x, y, s, s, s * 0.3f, HEX(0xFF7A59), HEX(0x7B5CFF), x, y, x + s, y + s);
    g_glow(g, x + s * 0.3f, y + s * 0.25f, s * 0.5f, rgba(255, 255, 255, 70));
    /* keel mark: hull + mast */
    float hull[] = {x + s * 0.2f, y + s * 0.58f, x + s * 0.8f, y + s * 0.58f, x + s * 0.62f, y + s * 0.78f, x + s * 0.38f, y + s * 0.78f};
    g_poly(g, hull, 4, WHITE);
    float sail[] = {x + s * 0.52f, y + s * 0.18f, x + s * 0.74f, y + s * 0.52f, x + s * 0.52f, y + s * 0.52f};
    g_poly(g, sail, 3, WHITE);
    float sail2[] = {x + s * 0.47f, y + s * 0.28f, x + s * 0.47f, y + s * 0.52f, x + s * 0.3f, y + s * 0.52f};
    g_poly(g, sail2, 3, rgba(255, 255, 255, 200));
    g_text_shadow(g, FONT_BLACK, s * 0.62f, x + s * 1.28f, y + s * 0.5f - font_cap(FONT_BLACK, s * 0.62f) * 0.5f, "KEEL", WHITE, ALIGN_LEFT, s * 0.06f);
    g_pop(g);
}

/* ---------------- Navigation bar ---------------- */
static const char *TAB_NAMES[TAB_COUNT] = {"Home", "Movies", "Live TV", "Music", "Photos", "Search", "Settings"};
static float nav_hl_x = -1, nav_hl_w;
static float nav_focus_amt;

void ui_nav(Gfx *g, int tab, float alpha) {
    if (alpha <= 0.01f) return;
    g_push(g);
    g_opacity(g, alpha);
    const float H = 64, Y = 36, fs = 25, pad = 28;
    static float scrim;
    scrim = approach(scrim, app.nav_scrim, app.dt, 10);
    if (scrim > 0.01f) {
        g_push(g);
        g_opacity(g, scrim);
        g_backdrop(g, 0, 0, SCREEN_W, 136, 0, 6 + app.set.glass * 2, rgba(30, 20, 60, 70), 1.4f);
        g_rect(g, 0, 135, SCREEN_W, 1.5f, 0, rgba(255, 255, 255, 50));
        g_pop(g);
    }
    float widths[TAB_COUNT], total = 0;
    for (int i = 0; i < TAB_COUNT; i++) {
        widths[i] = (i == TAB_SEARCH || i == TAB_SETTINGS) ? H - 12 : text_width(FONT_SEMIBOLD, fs, TAB_NAMES[i], 0) + pad * 2;
        total += widths[i];
    }
    total += 12;
    float x0 = SCREEN_W * 0.5f - total * 0.5f;
    ui_glass(g, x0, Y, total, H, H * 0.5f, 0.9f);
    int sel = app.nav_focused ? app.nav_index : tab;
    float tx = x0 + 6;
    float target_x = 0, target_w = 0;
    for (int i = 0; i < TAB_COUNT; i++) {
        if (i == sel) { target_x = tx; target_w = widths[i]; }
        tx += widths[i];
    }
    nav_focus_amt = approach(nav_focus_amt, app.nav_focused ? 1.f : 0.f, app.dt, 12);
    if (nav_hl_x < 0) { nav_hl_x = target_x; nav_hl_w = target_w; }
    nav_hl_x = approach(nav_hl_x, target_x, app.dt, 16);
    nav_hl_w = approach(nav_hl_w, target_w, app.dt, 16);
    /* highlight pill: glass when idle, solid white when focused */
    float hx = nav_hl_x, hw = nav_hl_w, hy = Y + 6, hh = H - 12;
    float f = nav_focus_amt;
    if (f > 0.02f) g_shadow(g, hx, hy + 4, hw, hh, hh * 0.5f, 18, rgba(0, 0, 0, (int)(50 * f)));
    g_rect(g, hx, hy, hw, hh, hh * 0.5f, col_lerp(rgba(255, 255, 255, 60), WHITE, f));
    tx = x0 + 6;
    for (int i = 0; i < TAB_COUNT; i++) {
        float cx = tx + widths[i] * 0.5f;
        float on = (i == sel) ? f : 0.f;
        uint32_t c = col_lerp(i == tab ? WHITE : rgba(255, 255, 255, 205), rgba(20, 18, 36, 255), on);
        if (i == TAB_SEARCH || i == TAB_SETTINGS) {
            g_icon(g, i == TAB_SEARCH ? IC_SEARCH : IC_GEAR, cx - 15, Y + H * 0.5f - 15, 30, c);
        } else {
            g_text(g, FONT_SEMIBOLD, fs, cx, Y + H * 0.5f - font_cap(FONT_SEMIBOLD, fs) * 0.5f, TAB_NAMES[i], c, ALIGN_CENTER, 0);
        }
        tx += widths[i];
    }
    ui_logo(g, SAFE_X, Y + 10, 44, 1);
    /* clock + avatar */
    char clk[16];
    app_clock_str(clk, sizeof clk);
    const Texture *av = g_profiles[app.profile].avatar;
    float ax = SCREEN_W - SAFE_X - 52;
    if (av && av->ready) {
        g_shadow(g, ax, Y + 10, 52, 52, 26, 14, rgba(0, 0, 0, 70));
        g_image(g, av, ax, Y + 6, 52, 52, 26, 1);
        g_stroke(g, ax, Y + 6, 52, 52, 26, 2, rgba(255, 255, 255, 160));
    }
    g_text_shadow(g, FONT_SEMIBOLD, 28, ax - 22, Y + H * 0.5f - font_cap(FONT_SEMIBOLD, 28) * 0.5f, clk, WHITE, ALIGN_RIGHT, 0);
    g_pop(g);
}

int ui_nav_input(int key) {
    if (!app.nav_focused) return 0;
    switch (key) {
    case K_LEFT:
        if (app.nav_index > 0) { app.nav_index--; app_tab(app.nav_index); app.nav_focused = 1; }
        return 1;
    case K_RIGHT:
        if (app.nav_index < TAB_COUNT - 1) { app.nav_index++; app_tab(app.nav_index); app.nav_focused = 1; }
        return 1;
    case K_DOWN: case K_OK:
        app.nav_focused = 0;
        return 1;
    case K_UP: return 1;
    }
    return 0;
}

/* ---------------- Buttons & widgets ---------------- */
float ui_button_width(float h, int icon, const char *label) {
    float fs = h * 0.36f;
    float w = h * 0.9f;
    if (label && *label) w += text_width(FONT_SEMIBOLD, fs, label, 0);
    if (icon >= 0) w += h * 0.42f + (label && *label ? h * 0.18f : 0);
    if (!label || !*label) w = h;
    return w;
}

float ui_button(Gfx *g, float x, float y, float h, int icon, const char *label, float f, int primary) {
    float w = ui_button_width(h, icon, label);
    float bx = x, by = y, bw = w, bh = h;
    ui_scale(&bx, &by, &bw, &bh, 1.f + 0.07f * f * motion());
    float r = bh * 0.5f;
    if (f > 0.01f) g_shadow(g, bx, by + 8 * f, bw, bh, r, 26, rgba(0, 0, 0, (int)(70 * f)));
    uint32_t idle = primary ? rgba(255, 255, 255, 70) : rgba(255, 255, 255, 38);
    g_rect(g, bx, by, bw, bh, r, col_lerp(idle, WHITE, f));
    if (f < 0.99f) g_stroke(g, bx, by, bw, bh, r, 1.5f, rgba(255, 255, 255, (int)(80 * (1 - f))));
    uint32_t c = col_lerp(WHITE, rgba(20, 18, 36, 255), f);
    float fs = h * 0.36f * (bh / h);
    float is = h * 0.42f * (bh / h);
    float cx = bx + bh * 0.45f;
    if (!label || !*label) cx = bx + (bw - is) * 0.5f;
    if (icon >= 0) {
        g_icon(g, icon, cx, by + (bh - is) * 0.5f, is, c);
        cx += is + bh * 0.18f;
    }
    if (label && *label) g_text(g, FONT_SEMIBOLD, fs, cx, by + bh * 0.5f - font_cap(FONT_SEMIBOLD, fs) * 0.5f, label, c, ALIGN_LEFT, 0);
    return w;
}

void ui_card_shadow(Gfx *g, float x, float y, float w, float h, float r, float f) {
    g_shadow(g, x, y + 8 + 16 * f, w, h, r, 22 + 34 * f, rgba(10, 0, 30, (int)(70 + 70 * f)));
}
void ui_focus_ring(Gfx *g, float x, float y, float w, float h, float r, float f) {
    if (f < 0.02f) return;
    g_stroke(g, x - 5, y - 5, w + 10, h + 10, r + 5, 3.5f, rgba(255, 255, 255, (int)(255 * f)));
}

void ui_tex(Gfx *g, const Texture *t, float x, float y, float w, float h, float r, float alpha) {
    if (t && t->ready) { g_image(g, t, x, y, w, h, r, alpha); return; }
    float ph = fmodf((float)app.time * 0.8f, 1.f);
    g_rect(g, x, y, w, h, r, rgba(255, 255, 255, (int)(40 * alpha)));
    g_push(g);
    g_clip(g, x, y, w, h);
    float sx = x - w + ph * w * 3;
    g_rect_hgrad(g, sx, y, w * 0.5f, h, 0, rgba(255, 255, 255, 0), rgba(255, 255, 255, (int)(45 * alpha)));
    g_rect_hgrad(g, sx + w * 0.5f, y, w * 0.5f, h, 0, rgba(255, 255, 255, (int)(45 * alpha)), rgba(255, 255, 255, 0));
    g_pop(g);
}

void ui_section(Gfx *g, float x, float y, const char *title, float alpha) {
    g_text_shadow(g, FONT_BOLD, 30, x, y, title, rgba(255, 255, 255, (int)(245 * alpha)), ALIGN_LEFT, 0);
}

void ui_progress(Gfx *g, float x, float y, float w, float h, float p, uint32_t fill) {
    g_rect(g, x, y, w, h, h * 0.5f, rgba(255, 255, 255, 70));
    if (p > 0) g_rect(g, x, y, fmaxf(h, w * sat(p)), h, h * 0.5f, fill);
}

void ui_stars(Gfx *g, float x, float y, float size, float score) {
    for (int i = 0; i < 5; i++) {
        float fill = clampf(score - i, 0, 1);
        g_icon(g, IC_STAR, x + i * size * 1.1f, y, size, rgba(255, 255, 255, 70));
        if (fill > 0) {
            g_push(g);
            g_clip(g, x + i * size * 1.1f, y, size * fill, size);
            g_icon(g, IC_STAR, x + i * size * 1.1f, y, size, HEX(0xFFD166));
            g_pop(g);
        }
    }
}

float ui_badge_width(const char *text) { return text_width(FONT_BOLD, 16, text, 1) + 20; }
void ui_badge(Gfx *g, float x, float y, const char *text, float alpha) {
    float w = ui_badge_width(text);
    g_rect(g, x, y, w, 30, 6, rgba(255, 255, 255, (int)(40 * alpha)));
    g_stroke(g, x, y, w, 30, 6, 1.5f, rgba(255, 255, 255, (int)(150 * alpha)));
    g_text(g, FONT_BOLD, 16, x + 10, y + 15 - font_cap(FONT_BOLD, 16) * 0.5f, text, rgba(255, 255, 255, (int)(240 * alpha)), ALIGN_LEFT, 1);
}

void ui_toggle(Gfx *g, float x, float y, int on, float f) {
    static float knob[64];
    (void)knob;
    float w = 68, h = 38;
    g_rect(g, x, y, w, h, h * 0.5f, on ? accent() : rgba(255, 255, 255, 70));
    float kx = on ? x + w - h + 4 : x + 4;
    g_shadow(g, kx, y + 6, h - 8, h - 8, (h - 8) * 0.5f, 8, rgba(0, 0, 0, 60));
    g_circle(g, kx + (h - 8) * 0.5f, y + h * 0.5f, (h - 8) * 0.5f, WHITE);
    (void)f;
}

void ui_spinner(Gfx *g, float cx, float cy, float r, float alpha) {
    float t = (float)app.time * 1.4f;
    for (int i = 0; i < 12; i++) {
        float a = i * PI_F / 6 + t * 2 * PI_F;
        float k = fmodf(i / 12.f + t, 1.f);
        g_line(g, cx + cosf(a) * r * 0.55f, cy + sinf(a) * r * 0.55f, cx + cosf(a) * r, cy + sinf(a) * r, r * 0.16f, rgba(255, 255, 255, (int)(255 * alpha * (0.2f + 0.8f * k))));
    }
}

void ui_fps_overlay(Gfx *g) {
    char buf[160];
    float x = SAFE_X, y = SCREEN_H - 110, w = 560, h = 64;
    ui_glass_dark(g, x, y, w, h, 18, 1);
    uint32_t c = app.fps >= 59.f ? HEX(0x3CE68C) : (app.fps >= 50 ? HEX(0xFFD166) : HEX(0xFF6B6B));
    g_circle(g, x + 28, y + h * 0.5f, 7, c);
    snprintf(buf, sizeof buf, "%.0f fps   %.1f ms   p99 %.1f ms   %d cmds", app.fps, app.frame_ms, app.frame_p99, app.cmds);
    g_text(g, FONT_SEMIBOLD, 22, x + 48, y + h * 0.5f - font_cap(FONT_SEMIBOLD, 22) * 0.5f, buf, WHITE, ALIGN_LEFT, 0);
}
