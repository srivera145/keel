/* KEEL OS — launch splash for third-party apps without a built-in screen. */
#include "ui.h"

static int ai;
static float t;

static void enter(int arg) {
    if (arg == -1) return;
    ai = clampi(arg, 0, g_napps - 1);
    t = 0;
}

static int input(int key, int ch) {
    (void)ch;
    if (key == K_OK && t > 2.6f) { app_back(); return 1; }
    return key != K_BACK && key != K_DEL && key != K_HOME;
}

static void update(float dt) { t += dt; }

static void draw(Gfx *g) {
    const AppInfo *a = &g_apps[ai];
    g_rect_grad(g, 0, 0, SCREEN_W, SCREEN_H, 0, a->c0 | 0xFF000000u, a->c1 | 0xFF000000u, 0, 0, SCREEN_W, SCREEN_H);
    g_glow(g, 500, 260, 700, rgba(255, 255, 255, 50));
    float k = ease_out_back(clampf(t / 0.7f, 0, 1));
    float s = 300 * (0.6f + 0.4f * k);
    float x = SCREEN_W / 2 - s / 2, y = 330 - s / 2 + (1 - k) * 60;
    g_shadow(g, x, y + 30, s, s, s * 0.22f, 70, rgba(0, 0, 0, 90));
    ui_tex(g, a->tile, x, y, s, s, s * 0.22f, 1);
    float ta = smoothstep(0.4f, 0.9f, t);
    g_push(g);
    g_opacity(g, ta);
    g_text_shadow(g, FONT_DISPLAY, 72, SCREEN_W / 2, 560, a->name, WHITE, ALIGN_CENTER, 0);
    g_text(g, FONT_MEDIUM, 30, SCREEN_W / 2, 660, a->tagline, rgba(255, 255, 255, 220), ALIGN_CENTER, 0);
    g_pop(g);
    if (t < 2.6f) {
        ui_spinner(g, SCREEN_W / 2, 800, 28, smoothstep(0.8f, 1.2f, t));
    } else {
        float b = smoothstep(2.6f, 3.1f, t);
        g_push(g);
        g_opacity(g, b);
        ui_glass(g, SCREEN_W / 2 - 420, 740, 840, 150, 40, 1);
        char msg[96];
        snprintf(msg, sizeof msg, "%s is coming soon to KEEL", a->name);
        g_text(g, FONT_BOLD, 30, SCREEN_W / 2, 776, msg, WHITE, ALIGN_CENTER, 0);
        g_text(g, FONT_MEDIUM, 22, SCREEN_W / 2, 832, "Press OK or Back to return Home", rgba(255, 255, 255, 200), ALIGN_CENTER, 0);
        g_pop(g);
    }
}

Screen scr_appsplash = {"appsplash", -1, enter, input, update, draw};
