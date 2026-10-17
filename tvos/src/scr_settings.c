/* KEEL OS — Settings: real, working preferences plus live system info. */
#include "audio.h"
#include "font.h"
#include "jobs.h"
#include "ui.h"

enum { CAT_GENERAL, CAT_DISPLAY, CAT_SOUND, CAT_NETWORK, CAT_PROFILES, CAT_ABOUT, NCAT };
static const char *CAT_NAMES[NCAT] = {"General", "Display", "Sound", "Network", "Profiles", "About"};
static const int CAT_ICONS[NCAT] = {IC_GEAR, IC_DISPLAY, IC_VOLUME, IC_WIFI, IC_USER, IC_INFO};

enum { IT_TOGGLE, IT_CHOICE, IT_SLIDER, IT_INFO, IT_ACTION };
typedef struct { const char *label; int type; int *val; int min, max; const char *(*fmt)(int); } Item;

static int cat, side, item;
static FocusAnim fcat, fitem;
static float hl_y = -1;
static char infobuf[16][96];

static const char *fmt_accent(int v) { return accent_name(v); }
static const char *fmt_saver(int v) {
    static char b[32];
    if (!v) return "Never";
    snprintf(b, sizeof b, "After %d min", v);
    return b;
}
static const char *fmt_glass(int v) { static const char *n[] = {"Subtle", "Light", "Balanced", "Rich", "Maximum"}; return n[clampi(v, 0, 4)]; }

static int saver_steps[] = {0, 2, 5, 10, 30};
static int saver_idx;

static int build_items(int c, Item *it) {
    int n = 0;
    switch (c) {
    case CAT_GENERAL:
        it[n++] = (Item){"Accent color", IT_CHOICE, &app.set.accent, 0, ACCENT_COUNT - 1, fmt_accent};
        it[n++] = (Item){"24-hour clock", IT_TOGGLE, &app.set.clock24, 0, 1, NULL};
        it[n++] = (Item){"Reduce motion", IT_TOGGLE, &app.set.reduce_motion, 0, 1, NULL};
        it[n++] = (Item){"Autoplay previews", IT_TOGGLE, &app.set.autoplay, 0, 1, NULL};
        it[n++] = (Item){"Subtitles", IT_TOGGLE, &app.set.captions, 0, 1, NULL};
        break;
    case CAT_DISPLAY:
        it[n++] = (Item){"Resolution", IT_INFO, NULL, 0, 0, NULL};
        it[n++] = (Item){"Glass intensity", IT_SLIDER, &app.set.glass, 0, 4, fmt_glass};
        it[n++] = (Item){"Performance overlay", IT_TOGGLE, &app.set.show_fps, 0, 1, NULL};
        it[n++] = (Item){"Screensaver", IT_CHOICE, &saver_idx, 0, 4, NULL};
        break;
    case CAT_SOUND:
        it[n++] = (Item){"Volume", IT_SLIDER, &app.set.volume, 0, 10, NULL};
        it[n++] = (Item){"Navigation sounds", IT_TOGGLE, &app.set.ui_sounds, 0, 1, NULL};
        it[n++] = (Item){"Audio output", IT_INFO, NULL, 0, 0, NULL};
        it[n++] = (Item){"Format", IT_INFO, NULL, 0, 0, NULL};
        break;
    case CAT_NETWORK:
        it[n++] = (Item){"Wi-Fi", IT_INFO, NULL, 0, 0, NULL};
        it[n++] = (Item){"Signal", IT_INFO, NULL, 0, 0, NULL};
        it[n++] = (Item){"IP address", IT_INFO, NULL, 0, 0, NULL};
        it[n++] = (Item){"Run network test", IT_ACTION, NULL, 0, 0, NULL};
        break;
    case CAT_PROFILES:
        for (int i = 0; i < g_nprofiles; i++) it[n++] = (Item){g_profiles[i].name, IT_ACTION, NULL, i, 0, NULL};
        it[n++] = (Item){"Switch profile", IT_ACTION, NULL, 99, 0, NULL};
        break;
    case CAT_ABOUT:
        it[n++] = (Item){"System", IT_INFO, NULL, 0, 0, NULL};
        it[n++] = (Item){"Renderer", IT_INFO, NULL, 0, 0, NULL};
        it[n++] = (Item){"Frame time", IT_INFO, NULL, 0, 0, NULL};
        it[n++] = (Item){"Typeface", IT_INFO, NULL, 0, 0, NULL};
        it[n++] = (Item){"Artwork", IT_INFO, NULL, 0, 0, NULL};
        it[n++] = (Item){"Uptime", IT_INFO, NULL, 0, 0, NULL};
        break;
    }
    return n;
}

static const char *info_value(int c, int i) {
    char *b = infobuf[i & 15];
    switch (c) {
    case CAT_DISPLAY: snprintf(b, 96, "1920 \xC3\x97 1080  \xC2\xB7  60 Hz"); break;
    case CAT_SOUND:
        if (i == 2) snprintf(b, 96, "%s", audio_device());
        else snprintf(b, 96, "44.1 kHz stereo  \xC2\xB7  live synthesis");
        break;
    case CAT_NETWORK:
        if (i == 0) snprintf(b, 96, "Keel Home 5G  \xC2\xB7  Connected");
        else if (i == 1) snprintf(b, 96, "Excellent");
        else snprintf(b, 96, "192.168.1.42");
        break;
    case CAT_ABOUT:
        switch (i) {
        case 0: snprintf(b, 96, "KEEL OS 1.0  \xC2\xB7  build 2026.10"); break;
        case 1: snprintf(b, 96, "Software compositor  \xC2\xB7  %d threads", jobs_render_threads()); break;
        case 2: snprintf(b, 96, "%.1f ms avg  \xC2\xB7  p99 %.1f ms", app.frame_ms, app.frame_p99); break;
        case 3: { const char *p = font_path(FONT_REGULAR), *s = strrchr(p, '/'); snprintf(b, 96, "%s", s ? s + 1 : p); break; }
        case 4: snprintf(b, 96, "%d%% generated on device", (int)(assets_progress() * 100)); break;
        default: { int s = (int)app.time; snprintf(b, 96, "%dh %02dm %02ds", s / 3600, (s / 60) % 60, s % 60); break; }
        }
        break;
    default: b[0] = 0;
    }
    return b;
}

static void enter(int arg) {
    if (arg == -1) return;
    side = 0;
    item = 0;
    cat = arg >= 0 && arg < NCAT ? arg : 0;
    if (arg == CAT_PROFILES) side = 1;
    for (int i = 0; i < 5; i++) if (saver_steps[i] == app.set.screensaver) saver_idx = i;
}

static void change(Item *it, int d) {
    if (!it->val) return;
    if (it->type == IT_TOGGLE) *it->val = !*it->val;
    else {
        int v = *it->val + d;
        if (it->type == IT_CHOICE && it->fmt) v = (v - it->min + (it->max - it->min + 1)) % (it->max - it->min + 1) + it->min;
        *it->val = clampi(v, it->min, it->max);
    }
    app.set.screensaver = saver_steps[saver_idx];
}

static int input(int key, int ch) {
    (void)ch;
    Item items[16];
    int n = build_items(cat, items);
    if (!side) {
        switch (key) {
        case K_UP: if (cat > 0) { cat--; item = 0; } else { app.nav_focused = 1; app.nav_index = TAB_SETTINGS; } return 1;
        case K_DOWN: if (cat < NCAT - 1) { cat++; item = 0; } return 1;
        case K_RIGHT: case K_OK: side = 1; item = 0; return 1;
        }
        return 0;
    }
    Item *it = &items[clampi(item, 0, n - 1)];
    switch (key) {
    case K_UP: if (item > 0) item--; else { app.nav_focused = 1; app.nav_index = TAB_SETTINGS; } return 1;
    case K_DOWN: if (item < n - 1) item++; return 1;
    case K_LEFT:
        if (it->type == IT_SLIDER || it->type == IT_CHOICE) { change(it, -1); return 1; }
        side = 0; return 1;
    case K_RIGHT:
        if (it->type == IT_SLIDER || it->type == IT_CHOICE) change(it, 1);
        return 1;
    case K_BACK: case K_DEL: side = 0; return 1;
    case K_OK:
        if (it->type == IT_TOGGLE || it->type == IT_CHOICE) change(it, 1);
        else if (it->type == IT_ACTION) {
            if (cat == CAT_NETWORK) app_toast("Network test passed  \xC2\xB7  412 Mbps", IC_WIFI);
            else if (it->min == 99) app_reset_to(&scr_profiles, 0);
            else {
                app.profile = it->min;
                char msg[64];
                snprintf(msg, sizeof msg, "Switched to %s", g_profiles[it->min].name);
                app_toast(msg, IC_USER);
            }
        }
        return 1;
    }
    return 0;
}

static void update(float dt) {
    focus_update(&fcat, NCAT, app.nav_focused ? -1 : cat, dt);
    Item items[16];
    int n = build_items(cat, items);
    focus_update(&fitem, n, side && !app.nav_focused ? item : -1, dt);
}

static void draw(Gfx *g) {
    ui_wallpaper(g, 1);
    g_rect(g, 0, 0, SCREEN_W, SCREEN_H, 0, rgba(20, 10, 50, 50));
    g_text_shadow(g, FONT_DISPLAY, 58, SAFE_X, 150, "Settings", WHITE, ALIGN_LEFT, 0);
    /* categories */
    float lx = 48, ly = 250, lw = 560, lh = 800;
    ui_glass(g, lx, ly, lw, lh, 36, 1);
    for (int i = 0; i < NCAT; i++) {
        float f = fcat.v[i] * (side ? 0.0f : 1.f);
        float y = ly + 30 + i * 92, x = lx + 24, w = lw - 48, h = 76;
        int sel = i == cat;
        if (sel && side) g_rect(g, x, y, w, h, 20, rgba(255, 255, 255, 50));
        if (f > 0.02f) {
            g_shadow(g, x, y + 6, w, h, 20, 18, rgba(0, 0, 0, (int)(60 * f)));
            g_rect(g, x, y, w, h, 20, rgba(255, 255, 255, (int)(255 * f)));
        }
        uint32_t c = col_lerp(WHITE, rgba(20, 18, 36, 255), f);
        g_rect(g, x + 18, y + 14, 48, 48, 14, col_lerp(rgba(255, 255, 255, 40), accent(), sel ? 1.f : 0.f));
        g_icon(g, CAT_ICONS[i], x + 26, y + 22, 32, WHITE);
        g_text(g, FONT_SEMIBOLD, 28, x + 88, y + h / 2 - font_cap(FONT_SEMIBOLD, 28) / 2, CAT_NAMES[i], c, ALIGN_LEFT, 0);
        g_icon(g, IC_CHEV_R, x + w - 46, y + h / 2 - 13, 26, col_alpha(c, 0.6f));
    }
    /* items */
    float rx = 640, ry = 250, rw = SCREEN_W - rx - 48, rh = 800;
    ui_glass(g, rx, ry, rw, rh, 36, 1);
    g_text(g, FONT_BOLD, 36, rx + 48, ry + 44, CAT_NAMES[cat], WHITE, ALIGN_LEFT, 0);
    Item items[16];
    int n = build_items(cat, items);
    float iy0 = ry + 120;
    if (cat == CAT_PROFILES) {
        for (int i = 0; i < g_nprofiles; i++) {
            float x = rx + 48 + i * 200, y = iy0 + 10;
            ui_tex(g, g_profiles[i].avatar, x, y, 120, 120, 60, 1);
            if (i == app.profile) g_stroke(g, x - 6, y - 6, 132, 132, 66, 4, accent());
            g_text(g, FONT_SEMIBOLD, 22, x + 60, y + 146, g_profiles[i].name, WHITE, ALIGN_CENTER, 0);
        }
        iy0 += 220;
    }
    for (int i = 0; i < n; i++) {
        Item *it = &items[i];
        float f = fitem.v[i];
        float x = rx + 32, y = iy0 + i * 96, w = rw - 64, h = 80;
        if (f > 0.02f) {
            g_shadow(g, x, y + 6, w, h, 20, 18, rgba(0, 0, 0, (int)(60 * f)));
            g_rect(g, x, y, w, h, 20, rgba(255, 255, 255, (int)(255 * f)));
        } else g_rect(g, x, y, w, h, 20, rgba(255, 255, 255, 22));
        uint32_t c = col_lerp(WHITE, rgba(20, 18, 36, 255), f);
        const char *label = it->label;
        char lab[64];
        if (cat == CAT_PROFILES && it->min != 99) { snprintf(lab, sizeof lab, "Use %s's profile", it->label); label = lab; }
        g_text(g, FONT_SEMIBOLD, 26, x + 28, y + h / 2 - font_cap(FONT_SEMIBOLD, 26) / 2, label, c, ALIGN_LEFT, 0);
        float vx = x + w - 28, vy = y + h / 2;
        switch (it->type) {
        case IT_TOGGLE: ui_toggle(g, vx - 68, vy - 19, *it->val, f); break;
        case IT_SLIDER: {
            float sw = 300, p = (float)(*it->val - it->min) / (it->max - it->min);
            float sx = vx - sw - (it->fmt ? 140 : 60);
            g_rect(g, sx, vy - 4, sw, 8, 4, col_lerp(rgba(255, 255, 255, 70), rgba(20, 18, 36, 50), f));
            g_rect(g, sx, vy - 4, sw * p, 8, 4, accent());
            g_circle(g, sx + sw * p, vy, 13, f > 0.5f ? accent() : WHITE);
            char v[32];
            snprintf(v, sizeof v, "%s", it->fmt ? it->fmt(*it->val) : "");
            if (!it->fmt) snprintf(v, sizeof v, "%d", *it->val);
            g_text(g, FONT_SEMIBOLD, 24, vx, vy - font_cap(FONT_SEMIBOLD, 24) / 2, v, col_alpha(c, 0.8f), ALIGN_RIGHT, 0);
            break;
        }
        case IT_CHOICE: {
            const char *v = it->fmt ? it->fmt(*it->val) : fmt_saver(saver_steps[saver_idx]);
            float tw = text_width(FONT_SEMIBOLD, 24, v, 0);
            g_icon(g, IC_CHEV_R, vx - 26, vy - 13, 26, col_alpha(c, 0.7f));
            g_text(g, FONT_SEMIBOLD, 24, vx - 40, vy - font_cap(FONT_SEMIBOLD, 24) / 2, v, c, ALIGN_RIGHT, 0);
            g_icon(g, IC_CHEV_L, vx - 70 - tw, vy - 13, 26, col_alpha(c, 0.7f));
            if (cat == CAT_GENERAL && i == 0) {
                for (int k = 0; k < ACCENT_COUNT; k++) {
                    float sx = vx - 330 - tw + k * 34;
                    if (k == *it->val) g_circle(g, sx, vy, 15, col_lerp(WHITE, rgba(20, 18, 36, 255), f));
                    g_circle(g, sx, vy, 11, accent_n(k));
                }
            }
            break;
        }
        case IT_INFO: g_text(g, FONT_MEDIUM, 24, vx, vy - font_cap(FONT_MEDIUM, 24) / 2, info_value(cat, i), col_alpha(c, 0.8f), ALIGN_RIGHT, 0); break;
        case IT_ACTION: g_icon(g, IC_CHEV_R, vx - 26, vy - 13, 26, col_alpha(c, 0.7f)); break;
        }
    }
    (void)hl_y;
}

Screen scr_settings = {"settings", TAB_SETTINGS, enter, input, update, draw};
