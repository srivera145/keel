/* KEEL OS — "Who's watching?" profile picker. */
#include "ui.h"

static int focus, row; /* row 0 avatars, row 1 manage button */
static float t;
static FocusAnim fa;
static float btn_f;

static void enter(int arg) {
    if (arg != -1) t = 0;
    row = 0;
    focus = app.profile;
}

static int input(int key, int ch) {
    (void)ch;
    switch (key) {
    case K_LEFT: if (row == 0 && focus > 0) focus--; return 1;
    case K_RIGHT: if (row == 0 && focus < g_nprofiles - 1) focus++; return 1;
    case K_DOWN: row = 1; return 1;
    case K_UP: row = 0; return 1;
    case K_OK:
        if (row == 0) {
            app.profile = focus;
            app.nav_focused = 0;
            app.nav_index = TAB_HOME;
            app_reset_to(&scr_home, 0);
        } else {
            app.profile = focus;
            app_reset_to(&scr_settings, 4);
        }
        return 1;
    case K_BACK: case K_DEL: return 1;
    }
    return 0;
}

static void update(float dt) {
    t += dt;
    focus_update(&fa, g_nprofiles, row == 0 ? focus : -1, dt);
    btn_f = approach(btn_f, row == 1 ? 1.f : 0.f, dt, 14);
}

static void draw(Gfx *g) {
    ui_wallpaper(g, 1);
    /* soft floating light */
    float tt = (float)app.time;
    g_glow(g, 960 + sinf(tt * 0.3f) * 300, 520 + cosf(tt * 0.23f) * 80, 380, rgba(255, 255, 255, 30));
    ui_logo(g, SAFE_X, 60, 48, 1);
    float ta = ease_out_cubic(t / 0.7f);
    g_push(g);
    g_opacity(g, ta);
    g_text_shadow(g, FONT_DISPLAY, 68, 960, 250 - (1 - ta) * 20, "Who's watching?", WHITE, ALIGN_CENTER, 0);
    g_pop(g);
    const float S = 230, gap = 90;
    float total = g_nprofiles * S + (g_nprofiles - 1) * gap;
    float x0 = 960 - total / 2, y0 = 420;
    for (int i = 0; i < g_nprofiles; i++) {
        float k = ease_out_back(clampf((t - 0.15f - i * 0.08f) / 0.7f, 0, 1));
        float f = fa.v[i];
        float x = x0 + i * (S + gap), y = y0 + (1 - k) * 80, w = S, h = S;
        ui_scale(&x, &y, &w, &h, 1.f + 0.12f * f * motion());
        g_push(g);
        g_opacity(g, clampf(k, 0, 1) * (0.78f + 0.22f * fmaxf(f, row == 1 ? 1.f : 0.f)));
        if (f > 0.02f) g_glow(g, x + w / 2, y + h / 2, w * 0.85f, rgba(255, 255, 255, (int)(70 * f)));
        g_shadow(g, x, y + 14 + 10 * f, w, h, w / 2, 40, rgba(20, 0, 50, (int)(90 + 50 * f)));
        ui_tex(g, g_profiles[i].avatar, x, y, w, h, w / 2, 1);
        if (f > 0.02f) g_stroke(g, x - 7, y - 7, w + 14, h + 14, w / 2 + 7, 5, rgba(255, 255, 255, (int)(255 * f)));
        g_text_shadow(g, f > 0.5f ? FONT_BOLD : FONT_SEMIBOLD, 32, x + w / 2, y + h + 44, g_profiles[i].name,
                      rgba(255, 255, 255, (int)(200 + 55 * f)), ALIGN_CENTER, 0);
        g_pop(g);
    }
    float bw = ui_button_width(60, IC_GEAR, "Manage Profiles");
    g_push(g);
    g_opacity(g, smoothstep(0.6f, 1.2f, t));
    ui_button(g, 960 - bw / 2, 860, 60, IC_GEAR, "Manage Profiles", btn_f, 0);
    g_pop(g);
}

Screen scr_profiles = {"profiles", -1, enter, input, update, draw};
