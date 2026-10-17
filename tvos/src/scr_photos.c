/* KEEL OS — Photos: library grid, full-screen viewer, Ken Burns slideshow
 * (which doubles as the screensaver). */
#include "ui.h"

#define PCOLS 4
static int sel, prow_btn;
static FocusAnim fa;
static float fbtn, vscroll;
static int bg_a = -1, bg_b = -1;
static float bg_fade = 1;

static void grid_enter(int arg) {
    if (arg == -1) return;
    prow_btn = 0;
    vscroll = 0;
}

static int grid_input(int key, int ch) {
    (void)ch;
    if (prow_btn) {
        switch (key) {
        case K_DOWN: prow_btn = 0; return 1;
        case K_UP: app.nav_focused = 1; app.nav_index = TAB_PHOTOS; return 1;
        case K_OK: app_push(&scr_viewer, 0x100 | sel); return 1;
        }
        return 1;
    }
    switch (key) {
    case K_LEFT: if (sel % PCOLS > 0) sel--; return 1;
    case K_RIGHT: if (sel % PCOLS < PCOLS - 1 && sel + 1 < g_nphotos) sel++; return 1;
    case K_UP: if (sel >= PCOLS) sel -= PCOLS; else prow_btn = 1; return 1;
    case K_DOWN: if (sel + PCOLS < g_nphotos) sel += PCOLS; return 1;
    case K_OK: app_push(&scr_viewer, sel); return 1;
    case K_PLAY: app_push(&scr_viewer, 0x100 | sel); return 1;
    }
    return 0;
}

static void grid_update(float dt) {
    focus_update(&fa, g_nphotos, prow_btn || app.nav_focused ? -1 : sel, dt);
    fbtn = approach(fbtn, prow_btn && !app.nav_focused ? 1.f : 0.f, dt, 14);
    int r = sel / PCOLS;
    vscroll = scroll_to(vscroll, prow_btn ? 0 : fmaxf(0, r * 300.f - 200), dt);
    app.nav_scrim = smoothstep(20, 120, vscroll);
    if (sel != bg_b) { bg_a = bg_b; bg_b = sel; bg_fade = 0; }
    bg_fade = fminf(1, bg_fade + dt / 0.6f);
}

static void pbg(Gfx *g, int i, float a) {
    if (i < 0) return;
    const Texture *t = g_photos[i].blur;
    if (t && t->ready) g_image(g, t, 0, 0, SCREEN_W, SCREEN_H, 0, a);
}

static void grid_draw(Gfx *g) {
    ui_wallpaper(g, 1);
    if (bg_a >= 0 && bg_fade < 1) pbg(g, bg_a, 1);
    pbg(g, bg_b, bg_a >= 0 ? ease_in_out_cubic(bg_fade) : 1);
    g_rect_vgrad(g, 0, 0, SCREEN_W, SCREEN_H, 0, rgba(255, 255, 255, 20), rgba(20, 10, 40, 110));
    g_push(g);
    g_translate(g, 0, -vscroll);
    g_text_shadow(g, FONT_DISPLAY, 58, SAFE_X, 150, "Photos", WHITE, ALIGN_LEFT, 0);
    char sub[64];
    snprintf(sub, sizeof sub, "%d photos  ·  Memories from this year", g_nphotos);
    g_text(g, FONT_MEDIUM, 22, SAFE_X + 4, 228, sub, rgba(255, 255, 255, 200), ALIGN_LEFT, 0);
    float bw = ui_button_width(62, IC_SLIDESHOW, "Play Slideshow");
    ui_button(g, SCREEN_W - SAFE_X - bw, 160, 62, IC_SLIDESHOW, "Play Slideshow", fbtn, 1);
    const float W = 400, H = 225, GX = 32, ROWH = 300;
    float x0 = (SCREEN_W - (PCOLS * W + (PCOLS - 1) * GX)) / 2, y0 = 300;
    for (int pass = 0; pass < 2; pass++)
        for (int i = 0; i < g_nphotos; i++) {
            float f = fa.v[i];
            if ((pass == 0) == (f > 0.02f)) continue;
            float x = x0 + (i % PCOLS) * (W + GX), y = y0 + (i / PCOLS) * ROWH, w = W, h = H;
            if (y - vscroll > SCREEN_H || y + h + 80 - vscroll < 0) continue;
            ui_scale(&x, &y, &w, &h, 1 + 0.08f * f * motion());
            ui_card_shadow(g, x, y, w, h, 18, f);
            ui_tex(g, g_photos[i].thumb, x, y, w, h, 18, 1);
            ui_focus_ring(g, x, y, w, h, 18, f);
            g_text(g, f > 0.5f ? FONT_BOLD : FONT_SEMIBOLD, 22, x + 4, y + h + 20, g_photos[i].title, rgba(255, 255, 255, (int)(190 + 65 * f)), ALIGN_LEFT, 0);
            if (f > 0.05f) {
                char pl[96];
                snprintf(pl, sizeof pl, "%s  ·  %s", g_photos[i].place, g_photos[i].date);
                g_text(g, FONT_MEDIUM, 18, x + 4, y + h + 52, pl, rgba(255, 255, 255, (int)(200 * f)), ALIGN_LEFT, 0);
            }
        }
    g_pop(g);
}

Screen scr_photos = {"photos", TAB_PHOTOS, grid_enter, grid_input, grid_update, grid_draw};

/* ======================= Viewer / slideshow ======================= */
static int cur, prev = -1, show, saver, vpaused;
static float slide_t, trans, osd_t, dir;

static void viewer_enter(int arg) {
    if (arg == -1) return;
    cur = arg & 0xFF;
    show = (arg & 0x100) != 0;
    saver = (arg & 0x200) != 0;
    prev = -1;
    slide_t = 0;
    trans = 1;
    osd_t = 0;
    vpaused = 0;
}

static void go(int d) {
    prev = cur;
    cur = (cur + d + g_nphotos) % g_nphotos;
    trans = 0;
    dir = (float)d;
    slide_t = 0;
}

static int viewer_input(int key, int ch) {
    (void)ch;
    osd_t = 0;
    if (saver) { app_back(); return 1; }
    switch (key) {
    case K_LEFT: go(-1); return 1;
    case K_RIGHT: go(1); return 1;
    case K_OK: case K_PLAY:
        if (!show) { show = 1; slide_t = 0; }
        else vpaused = !vpaused;
        return 1;
    case K_UP: case K_DOWN: return 1;
    }
    return 0;
}

static void viewer_update(float dt) {
    osd_t += dt;
    trans = fminf(1, trans + dt / (show ? 1.4f : 0.45f));
    if (!vpaused) slide_t += dt;
    if (show && !vpaused && slide_t > 6.5f) go(1);
}

static void photo_draw(Gfx *g, int i, float alpha, float kt, float xoff) {
    const Photo *p = &g_photos[i];
    const Texture *t = p->image && p->image->ready ? p->image : p->thumb;
    if (!t || !t->ready) { ui_spinner(g, SCREEN_W / 2, SCREEN_H / 2, 30, alpha); return; }
    if (show) {
        float z = 1.02f + 0.12f * (kt / 8.f) * motion();
        float sw = t->w / z, sh = t->h / z;
        float fx = (i & 1) ? kt / 8.f : 1 - kt / 8.f, fy = (i & 2) ? 0.3f : 0.7f;
        g_image_src(g, t, (t->w - sw) * clampf(fx, 0, 1), (t->h - sh) * fy, sw, sh, xoff, 0, SCREEN_W, SCREEN_H, 0, alpha);
    } else g_image(g, t, xoff, 0, SCREEN_W, SCREEN_H, 0, alpha);
}

static void viewer_draw(Gfx *g) {
    g_rect(g, 0, 0, SCREEN_W, SCREEN_H, 0, BLACK);
    float e = ease_in_out_cubic(trans);
    if (show) {
        if (prev >= 0 && trans < 1) photo_draw(g, prev, 1, slide_t + 6.5f, 0);
        photo_draw(g, cur, prev >= 0 ? e : 1, slide_t, 0);
    } else {
        if (prev >= 0 && trans < 1) photo_draw(g, prev, 1, 0, -dir * e * SCREEN_W);
        photo_draw(g, cur, 1, 0, prev >= 0 && trans < 1 ? dir * (1 - e) * SCREEN_W : 0);
    }
    const Photo *p = &g_photos[cur];
    float oa = show ? 1 - smoothstep(4.f, 5.f, slide_t) : 1 - smoothstep(3.5f, 4.2f, osd_t);
    if (saver) {
        char clk[16];
        app_clock_str(clk, sizeof clk);
        g_text_shadow(g, FONT_THIN, 120, SCREEN_W - SAFE_X, 90, clk, WHITE, ALIGN_RIGHT, 0);
    }
    if (oa > 0.01f) {
        g_push(g);
        g_opacity(g, oa);
        g_rect_vgrad(g, 0, SCREEN_H - 300, SCREEN_W, 300, 0, rgba(0, 0, 0, 0), rgba(0, 0, 0, 120));
        float w = fmaxf(text_width(FONT_BOLD, 40, p->title, 0), text_width(FONT_MEDIUM, 24, p->place, 0) + 120) + 80;
        ui_glass(g, SAFE_X - 24, SCREEN_H - 230, w, 150, 32, 0.9f);
        g_text(g, FONT_BOLD, 40, SAFE_X + 8, SCREEN_H - 200, p->title, WHITE, ALIGN_LEFT, 0);
        char pl[96];
        snprintf(pl, sizeof pl, "%s  ·  %s", p->place, p->date);
        g_text(g, FONT_MEDIUM, 24, SAFE_X + 8, SCREEN_H - 136, pl, rgba(255, 255, 255, 220), ALIGN_LEFT, 0);
        if (!saver) {
            char idx[32];
            snprintf(idx, sizeof idx, "%d / %d", cur + 1, g_nphotos);
            g_text(g, FONT_SEMIBOLD, 24, SCREEN_W - SAFE_X, SCREEN_H - 130, idx, rgba(255, 255, 255, 220), ALIGN_RIGHT, 0);
            if (show) {
                ui_glass_dark(g, SCREEN_W - SAFE_X - 240, 60, 240, 60, 30, 1);
                g_icon(g, vpaused ? IC_PAUSE : IC_SLIDESHOW, SCREEN_W - SAFE_X - 220, 72, 36, WHITE);
                g_text(g, FONT_SEMIBOLD, 22, SCREEN_W - SAFE_X - 170, 90 - font_cap(FONT_SEMIBOLD, 22) / 2, vpaused ? "Paused" : "Slideshow", WHITE, ALIGN_LEFT, 0);
            } else {
                g_text(g, FONT_MEDIUM, 20, SCREEN_W - SAFE_X, SCREEN_H - 90, "OK  start slideshow", rgba(255, 255, 255, 170), ALIGN_RIGHT, 0);
            }
        }
        g_pop(g);
    }
    if (show && !saver) {
        for (int i = 0; i < g_nphotos; i++) {
            float on = i == cur;
            g_rect(g, SCREEN_W / 2 - g_nphotos * 12 + i * 24, SCREEN_H - 40, on ? 16 : 8, 8, 4, rgba(255, 255, 255, on ? 255 : 100));
        }
    }
}

Screen scr_viewer = {"viewer", -1, viewer_enter, viewer_input, viewer_update, viewer_draw};
