/* KEEL OS — Home: cinematic hero, Continue Watching, Trending Apps, Top Picks. */
#include "ui.h"

unsigned char g_mylist[64];

enum { ROW_HERO, ROW_CONT, ROW_APPS, ROW_PICKS, NROWS };
static int row, col[NROWS];
static FocusAnim fa[NROWS];
static float hscroll[NROWS], vscroll;
static int hero, hero_prev = -1;
static float hero_t, hero_fade = 1;
static float t_enter;

static const int PICKS[] = {2, 8, 14, 17, 3, 21, 9, 18, 5, 12, 23, 19};
#define NPICKS COUNT(PICKS)

static const float ROW_Y[NROWS] = {690, 880, 1270, 1620};      /* item top, page coords */
static const float ROW_SCROLL[NROWS] = {0, 600, 990, 1395};

static int row_len(int r) {
    switch (r) {
    case ROW_HERO: return 3;
    case ROW_CONT: return g_ncontinue;
    case ROW_APPS: return g_napps;
    default: return NPICKS;
    }
}

static void enter(int arg) {
    if (arg == -1) return;
    t_enter = 0;
    if (arg == 0) { row = ROW_HERO; vscroll = 0; memset(col, 0, sizeof col); memset(hscroll, 0, sizeof hscroll); }
}

static void set_hero(int i) {
    if (i == hero) return;
    hero_prev = hero;
    hero = (i + g_nfeatured) % g_nfeatured;
    hero_fade = 0;
    hero_t = 0;
}

static void activate(void) {
    int c = col[row];
    switch (row) {
    case ROW_HERO: {
        int m = g_featured[hero];
        if (c == 0) app_push(&scr_player, m);
        else if (c == 1) app_push(&scr_details, m);
        else {
            g_mylist[m] = !g_mylist[m];
            app_toast(g_mylist[m] ? "Added to My List" : "Removed from My List", g_mylist[m] ? IC_CHECK : IC_PLUS);
        }
        break;
    }
    case ROW_CONT: app_push(&scr_player, g_continue[c].movie); break;
    case ROW_APPS:
        if (g_apps[c].target >= 0) { app.nav_focused = 0; app_tab(g_apps[c].target); }
        else app_push(&scr_appsplash, c);
        break;
    case ROW_PICKS: app_push(&scr_details, PICKS[c]); break;
    }
}

static int input(int key, int ch) {
    (void)ch;
    hero_t = fminf(hero_t, 4.f);
    switch (key) {
    case K_UP:
        if (row == ROW_HERO) { app.nav_focused = 1; app.nav_index = TAB_HOME; }
        else row--;
        return 1;
    case K_DOWN: if (row < NROWS - 1) row++; return 1;
    case K_LEFT:
        if (col[row] > 0) col[row]--;
        else if (row == ROW_HERO) set_hero(hero - 1);
        return 1;
    case K_RIGHT:
        if (col[row] < row_len(row) - 1) col[row]++;
        else if (row == ROW_HERO) set_hero(hero + 1);
        return 1;
    case K_OK: activate(); return 1;
    case K_PLAY: if (row == ROW_HERO) { app_push(&scr_player, g_featured[hero]); return 1; } return 0;
    case K_BACK: case K_DEL:
        if (row != ROW_HERO) { row = ROW_HERO; return 1; }
        return 0;
    }
    return 0;
}

static void update(float dt) {
    t_enter += dt;
    hero_t += dt;
    hero_fade = fminf(1.f, hero_fade + dt / 1.1f);
    if (hero_t > 9.f && row == ROW_HERO && !app.nav_focused) set_hero(hero + 1);
    vscroll = scroll_to(vscroll, ROW_SCROLL[row], dt);
    for (int r = 0; r < NROWS; r++) focus_update(&fa[r], row_len(r), (r == row && !app.nav_focused) ? col[r] : -1, dt);
    /* horizontal scroll keeps the focused card in view */
    const float item_w[NROWS] = {0, 456, 246, 276};
    for (int r = 1; r < NROWS; r++) {
        float target = fmaxf(0, col[r] * item_w[r] - item_w[r] * 1.0f);
        float maxs = fmaxf(0, row_len(r) * item_w[r] - (SCREEN_W - 2 * SAFE_X) + 30);
        target = fminf(target, maxs);
        hscroll[r] = scroll_to(hscroll[r], target, dt);
    }
}

static void hero_image(Gfx *g, int idx, float alpha, float kt) {
    const Movie *m = &g_movies[g_featured[idx]];
    const Texture *t = m->backdrop;
    if (!(t && t->ready)) { t = m->blur; if (!(t && t->ready)) return; }
    float z = 1.f + 0.06f * ease_in_out_cubic(clampf(kt / 10.f, 0, 1)) * motion();
    float sw = t->w / z, sh = t->h / z;
    float fx = (idx & 1) ? 0.7f : 0.3f;
    g_image_src(g, t, (t->w - sw) * fx, (t->h - sh) * 0.4f, sw, sh, 0, -vscroll * 0.35f, SCREEN_W, SCREEN_H, 0, alpha);
}

static void draw_hero_info(Gfx *g, float a) {
    const Movie *m = &g_movies[g_featured[hero]];
    float in = ease_out_cubic(hero_fade * 1.6f);
    g_push(g);
    g_opacity(g, a * in);
    g_translate(g, (1 - in) * 40 * motion(), -vscroll);
    float x = SAFE_X;
    /* label */
    float lw = text_width(FONT_BOLD, 17, "KEEL ORIGINAL", 3) + 52;
    g_rect(g, x, 300, lw, 36, 18, rgba(255, 255, 255, 46));
    g_icon(g, IC_SPARKLE, x + 12, 307, 22, HEX(0xFFD166));
    g_text(g, FONT_BOLD, 17, x + 40, 318 - font_cap(FONT_BOLD, 17) / 2, "KEEL ORIGINAL", WHITE, ALIGN_LEFT, 3);
    /* title */
    float size = 104;
    float tw = text_width(FONT_BLACK, size, m->title, 0);
    if (tw > 1000) size *= 1000 / tw;
    g_text_shadow(g, FONT_BLACK, size, x - 4, 370, m->title, WHITE, ALIGN_LEFT, 0);
    /* meta */
    char meta[128];
    snprintf(meta, sizeof meta, "%s  ·  %s  ·  %s", m->year, m->genre, m->runtime);
    float my = 370 + font_cap(FONT_BLACK, size) + 40;
    float mw = g_text_shadow(g, FONT_SEMIBOLD, 26, x, my, meta, rgba(255, 255, 255, 235), ALIGN_LEFT, 0);
    float bx = x + mw + 22;
    const char *badges[3] = {m->rating, "4K", "HDR"};
    for (int i = 0; i < 3; i++) { ui_badge(g, bx, my - 5, badges[i], 1); bx += ui_badge_width(badges[i]) + 10; }
    g_text_wrap(g, FONT_REGULAR, 26, x, my + 56, 840, 38, 2, m->synopsis, rgba(255, 255, 255, 225));
    g_pop(g);
    /* buttons */
    g_push(g);
    g_opacity(g, a);
    g_translate(g, 0, -vscroll);
    float bxx = x;
    bxx += ui_button(g, bxx, ROW_Y[ROW_HERO], 68, IC_PLAY, "Play", fa[ROW_HERO].v[0], 1) + 22;
    bxx += ui_button(g, bxx, ROW_Y[ROW_HERO], 68, IC_INFO, "More Info", fa[ROW_HERO].v[1], 0) + 22;
    ui_button(g, bxx, ROW_Y[ROW_HERO], 68, g_mylist[g_featured[hero]] ? IC_CHECK : IC_PLUS, "My List", fa[ROW_HERO].v[2], 0);
    /* page dots */
    for (int i = 0; i < g_nfeatured; i++) {
        float on = i == hero ? 1.f : 0.f;
        float w = 12 + 26 * on;
        float dx = SCREEN_W - SAFE_X - (g_nfeatured - i) * 24 - (on ? 26 : 0);
        (void)w;
        g_rect(g, dx, ROW_Y[ROW_HERO] + 28, i == hero ? 36 : 12, 12, 6, rgba(255, 255, 255, i == hero ? 255 : 110));
        if (i == hero) {
            float p = clampf(hero_t / 9.f, 0, 1);
            g_rect(g, dx, ROW_Y[ROW_HERO] + 28, 36 * p, 12, 6, accent());
        }
    }
    g_pop(g);
}

static void draw(Gfx *g) {
    g_rect(g, 0, 0, SCREEN_W, SCREEN_H, 0, BLACK);
    if (hero_prev >= 0 && hero_fade < 1) hero_image(g, hero_prev, 1, hero_t + 9);
    hero_image(g, hero, hero_prev >= 0 ? ease_in_out_cubic(hero_fade) : 1, hero_t);
    /* cinematic scrims */
    g_rect_hgrad(g, 0, 0, 1250, SCREEN_H, 0, rgba(10, 6, 30, 165), rgba(10, 6, 30, 0));
    g_rect_vgrad(g, 0, 560, SCREEN_W, 520, 0, rgba(10, 6, 30, 0), rgba(10, 6, 30, 150));
    g_rect_vgrad(g, 0, 0, SCREEN_W, 220, 0, rgba(10, 6, 30, 90), rgba(10, 6, 30, 0));
    float hero_a = 1.f - smoothstep(80, 420, vscroll);
    if (hero_a > 0.01f) draw_hero_info(g, hero_a);
    /* frosted sheet behind the rows */
    float sheet = 800 - vscroll;
    float sk = smoothstep(0, 400, vscroll);
    float sy = fmaxf(sheet, 130);
    if (sheet < SCREEN_H) ui_glass(g, 24, sy, SCREEN_W - 48, SCREEN_H - sy + 60, 44, 0.45f + 0.55f * sk);
    g_push(g);
    /* rows live inside the sheet: clip what scrolls past its top edge */
    g_clip(g, 0, sy + 2, SCREEN_W, SCREEN_H);
    g_translate(g, 0, -vscroll);
    /* Continue Watching */
    float y = ROW_Y[ROW_CONT];
    ui_section(g, SAFE_X, y - 54, "Continue Watching", 1);
    g_push(g);
    g_translate(g, -hscroll[ROW_CONT], 0);
    for (int i = 0; i < g_ncontinue; i++) {
        const Movie *m = &g_movies[g_continue[i].movie];
        float f = fa[ROW_CONT].v[i];
        float x = SAFE_X + i * 456, cy = y, w = 420, h = 236;
        if (x - hscroll[ROW_CONT] > SCREEN_W || x + w - hscroll[ROW_CONT] < 0) continue;
        ui_scale(&x, &cy, &w, &h, 1.f + 0.08f * f * motion());
        ui_card_shadow(g, x, cy, w, h, 22, f);
        ui_tex(g, m->thumb ? m->thumb : m->blur, x, cy, w, h, 22, 1);
        g_rect_vgrad(g, x, cy + h * 0.45f, w, h * 0.55f, 22, rgba(0, 0, 0, 0), rgba(0, 0, 0, 160));
        g_text_fit(g, FONT_BOLD, 26, x + 22, cy + h - 86, m->title, WHITE, w - 44, ALIGN_LEFT);
        g_text(g, FONT_MEDIUM, 19, x + 22, cy + h - 48, g_continue[i].label, rgba(255, 255, 255, 210), ALIGN_LEFT, 0);
        ui_progress(g, x + 22, cy + h - 22, w - 44, 7, g_continue[i].progress, accent());
        if (f > 0.05f) {
            g_circle(g, x + w / 2, cy + h / 2 - 18, 36 * f, rgba(255, 255, 255, (int)(230 * f)));
            g_icon(g, IC_PLAY, x + w / 2 - 15, cy + h / 2 - 33, 32, rgba(20, 18, 36, (int)(255 * f)));
        }
        ui_focus_ring(g, x, cy, w, h, 22, f);
    }
    g_pop(g);
    /* Trending Apps */
    y = ROW_Y[ROW_APPS];
    ui_section(g, SAFE_X, y - 54, "Trending Apps", 1);
    g_push(g);
    g_translate(g, -hscroll[ROW_APPS], 0);
    for (int i = 0; i < g_napps; i++) {
        float f = fa[ROW_APPS].v[i];
        float x = SAFE_X + i * 246, ay = y, w = 210, h = 210;
        if (x - hscroll[ROW_APPS] > SCREEN_W || x + w - hscroll[ROW_APPS] < 0) continue;
        ui_scale(&x, &ay, &w, &h, 1.f + 0.1f * f * motion());
        ui_card_shadow(g, x, ay, w, h, 48, f);
        ui_tex(g, g_apps[i].tile, x, ay, w, h, 48 * w / 210, 1);
        g_rect_vgrad(g, x, ay, w, h * 0.5f, 48 * w / 210, rgba(255, 255, 255, (int)(40 * f)), rgba(255, 255, 255, 0));
        ui_focus_ring(g, x, ay, w, h, 48 * w / 210, f);
        float la = 0.65f + 0.35f * f;
        g_text(g, f > 0.5f ? FONT_BOLD : FONT_SEMIBOLD, 22, x + w / 2, ay + h + 22 + 8 * f, g_apps[i].name, rgba(255, 255, 255, (int)(255 * la)), ALIGN_CENTER, 0);
        if (f > 0.05f) g_text(g, FONT_MEDIUM, 17, x + w / 2, ay + h + 56 + 8 * f, g_apps[i].tagline, rgba(255, 255, 255, (int)(180 * f)), ALIGN_CENTER, 0);
    }
    g_pop(g);
    /* Top Picks */
    y = ROW_Y[ROW_PICKS];
    char hdr[64];
    snprintf(hdr, sizeof hdr, "Top Picks for %s", g_profiles[app.profile].name);
    ui_section(g, SAFE_X, y - 54, hdr, 1);
    g_push(g);
    g_translate(g, -hscroll[ROW_PICKS], 0);
    for (int i = 0; i < NPICKS; i++) {
        const Movie *m = &g_movies[PICKS[i]];
        float f = fa[ROW_PICKS].v[i];
        float x = SAFE_X + i * 276, py = y, w = 240, h = 360;
        if (x - hscroll[ROW_PICKS] > SCREEN_W || x + w - hscroll[ROW_PICKS] < 0) continue;
        ui_scale(&x, &py, &w, &h, 1.f + 0.08f * f * motion());
        ui_card_shadow(g, x, py, w, h, 18, f);
        ui_tex(g, m->poster, x, py, w, h, 18, 1);
        ui_focus_ring(g, x, py, w, h, 18, f);
    }
    g_pop(g);
    g_pop(g);
}

Screen scr_home = {"home", TAB_HOME, enter, input, update, draw};
