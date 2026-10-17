/* KEEL OS — application shell: navigation stack, transitions, toasts. */
#include "app.h"
#include "audio.h"
#include "ui.h"
#include <time.h>

App app;

static Screen *stack[16];
static int stack_args[16];
static int depth;

/* Transition: snapshot of the outgoing frame, faded and scaled away. */
static Texture *snap;
static float trans_t = 1.f;
static int trans_dir;
static const uint32_t *last_px;
static int last_stride;

static char toast_msg[128];
static int toast_icon;
static float toast_t = 99.f;

static const uint32_t ACCENTS[ACCENT_COUNT] = {0xFFFF6B6B, 0xFF8B5CF6, 0xFF3B9EFF, 0xFF1ED6A0, 0xFFFFB020};
static const char *ACCENT_NAMES[ACCENT_COUNT] = {"Coral", "Violet", "Ocean", "Mint", "Amber"};
uint32_t accent(void) { return ACCENTS[app.set.accent % ACCENT_COUNT]; }
uint32_t accent_n(int i) { return ACCENTS[i % ACCENT_COUNT]; }
const char *accent_name(int i) { return ACCENT_NAMES[i % ACCENT_COUNT]; }
float motion(void) { return app.set.reduce_motion ? 0.25f : 1.f; }

static Screen *tab_screens[TAB_COUNT] = {&scr_home, &scr_movies, &scr_live, &scr_music, &scr_photos, &scr_search, &scr_settings};

void app_init(void) {
    app.set = (Settings){0, 0, 0, 2, 0, 1, 7, 0, 1, 5};
    app.profile = 0;
    snap = tex_new(SCREEN_W, SCREEN_H);
    snap->opaque = 1;
    depth = 0;
    app_push(&scr_boot, 0);
    trans_t = 1.f;
}

/* Snapshot the last presented frame (still intact: double-buffered, or
 * not yet overwritten because this frame hasn't rendered). */
static void begin_transition(int dir) {
    trans_dir = dir;
    if (!last_px) return;
    for (int y = 0; y < SCREEN_H; y++) memcpy(snap->px + (size_t)y * SCREEN_W, last_px + (size_t)y * last_stride, SCREEN_W * 4);
    snap->ready = 1;
    trans_t = 0.f;
}

void app_frame_done(const uint32_t *px, int stride) {
    last_px = px;
    last_stride = stride;
}

Screen *app_current(void) { return depth ? stack[depth - 1] : NULL; }

static void enter(Screen *s, int arg) {
    app.idle_since = app.time;
    if (s->enter) s->enter(arg);
}

void app_push(Screen *s, int arg) {
    if (depth >= 16) return;
    if (depth) begin_transition(1);
    stack[depth] = s;
    stack_args[depth] = arg;
    depth++;
    enter(s, arg);
}

void app_back(void) {
    if (depth <= 1) return;
    begin_transition(-1);
    depth--;
    Screen *s = stack[depth - 1];
    if (s->enter) s->enter(-1); /* -1: returning, keep state */
}

void app_reset_to(Screen *s, int arg) {
    begin_transition(1);
    depth = 0;
    stack[depth] = s;
    stack_args[depth] = arg;
    depth++;
    enter(s, arg);
}

void app_tab(int tab) {
    tab = clampi(tab, 0, TAB_COUNT - 1);
    Screen *s = tab_screens[tab];
    app.nav_index = tab;
    if (depth == 1 && stack[0] == s) return;
    begin_transition(0);
    depth = 0;
    stack[depth++] = s;
    enter(s, 0);
}

void app_toast(const char *msg, int icon) {
    snprintf(toast_msg, sizeof toast_msg, "%s", msg);
    toast_icon = icon;
    toast_t = 0;
}

int app_wall_minutes(void) {
    time_t now = time(NULL);
    struct tm tmv;
    localtime_r(&now, &tmv);
    return tmv.tm_hour * 60 + tmv.tm_min;
}
void app_clock_str(char *buf, int n) {
    int m = app_wall_minutes();
    int h = m / 60, mm = m % 60;
    if (app.set.clock24) snprintf(buf, n, "%02d:%02d", h, mm);
    else snprintf(buf, n, "%d:%02d", h % 12 == 0 ? 12 : h % 12, mm);
}

void app_input(int key, int ch) {
    app.idle_since = app.time;
    Screen *s = app_current();
    if (!s) return;
    if (key == K_FPS) { app.set.show_fps = !app.set.show_fps; return; }
    if (key == K_QUIT) { app.quit = 1; return; }
    if (s == &scr_boot) return;
    if (key == K_HOME && s != &scr_profiles) {
        app.nav_focused = 0;
        if (depth == 1 && s == &scr_home) scr_home.enter(0); /* already home: back to the top */
        else app_tab(TAB_HOME);
        return;
    }
    if (app.set.ui_sounds && key >= K_UP && key <= K_RIGHT) sfx_play(SFX_MOVE);
    if (app.set.ui_sounds && key == K_OK) sfx_play(SFX_SELECT);
    if (s->tab >= 0 && app.nav_focused && ui_nav_input(key)) return;
    if (s->input && s->input(key, ch)) return;
    /* Unhandled keys */
    if (key == K_CHAR && (ch == 'f' || ch == 'F')) { app.set.show_fps = !app.set.show_fps; return; }
    if (key == K_PLAY || (key == K_CHAR && ch == ' ')) { if (audio_playing()) audio_toggle_pause(); return; }
    if (key == K_BACK || key == K_DEL) {
        if (depth > 1) { app_back(); return; }
        if (s->tab >= 0) {
            if (!app.nav_focused) { app.nav_focused = 1; app.nav_index = s->tab; return; }
            if (s->tab != TAB_HOME) { app_tab(TAB_HOME); app.nav_focused = 1; return; }
            app.nav_focused = 0;
            app_reset_to(&scr_profiles, 0);
        }
    }
}

void app_update(float dt) {
    app.dt = dt;
    if (trans_t < 1.f) trans_t = fminf(1.f, trans_t + dt / (app.set.reduce_motion ? 0.15f : 0.42f));
    toast_t += dt;
    Screen *s = app_current();
    app.nav_scrim = 0;
    if (s && s->update) s->update(dt);
    /* Screensaver: photo slideshow after the configured idle time. */
    if (app.set.screensaver > 0 && s && app.time - app.idle_since > app.set.screensaver * 60.0 &&
        s != &scr_boot && s != &scr_player && s != &scr_viewer && s != &scr_live_full && s != &scr_nowplaying) {
        app.idle_since = app.time;
        app_push(&scr_viewer, 0x300 | (int)(hash32((uint32_t)app.time) % 16));
    }
}

void ui_toast_draw(Gfx *g) {
    if (toast_t > 3.2f) return;
    float in = ease_out_cubic(toast_t / 0.35f), out = 1.f - smoothstep(2.7f, 3.2f, toast_t);
    float a = fminf(in, out);
    float w = text_width(FONT_SEMIBOLD, 24, toast_msg, 0) + 110, h = 72;
    float x = SCREEN_W - SAFE_X - w, y = 120 - (1 - in) * 30;
    g_push(g);
    g_opacity(g, a);
    ui_glass(g, x, y, w, h, 24, 1);
    g_circle(g, x + 40, y + h * 0.5f, 20, accent());
    g_icon(g, toast_icon, x + 28, y + h * 0.5f - 12, 24, WHITE);
    g_text(g, FONT_SEMIBOLD, 24, x + 76, y + h * 0.5f - font_cap(FONT_SEMIBOLD, 24) * 0.5f, toast_msg, WHITE, ALIGN_LEFT, 0);
    g_pop(g);
}

void app_draw(Gfx *g) {
    Screen *s = app_current();
    if (!s) return;
    if (s->draw) s->draw(g);
    if (s->tab >= 0) ui_nav(g, s->tab, 1.f);
    if (trans_t < 1.f && snap->ready) {
        float e = ease_out_cubic(trans_t);
        float a = 1.f - e;
        float sc = trans_dir == 0 ? 1.f : (trans_dir > 0 ? 1.f + 0.06f * e : 1.f - 0.05f * e);
        sc = 1.f + (sc - 1.f) * motion();
        float w = SCREEN_W * sc, h = SCREEN_H * sc;
        g_image(g, snap, (SCREEN_W - w) * 0.5f, (SCREEN_H - h) * 0.5f, w, h, 0, a);
    }
    ui_toast_draw(g);
    if (app.set.show_fps) ui_fps_overlay(g);
}
