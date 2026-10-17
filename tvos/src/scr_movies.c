/* KEEL OS — Movies: poster grid with genre filters, details page, player. */
#include "audio.h"
#include "film.h"
#include "ui.h"

extern unsigned char g_mylist[64];

/* ======================= Grid ======================= */
#define COLS 6
static const char *genres[24];
static int ngenres;
static int chip, row, colx, nitems, items[64];
static int on_chips;
static FocusAnim fa, fchips;
static float vscroll, chip_scroll;
static int bg_cur = -1, bg_prev = -1;
static float bg_fade = 1;

static void build_genres(void) {
    if (ngenres) return;
    genres[ngenres++] = "All";
    genres[ngenres++] = "My List";
    for (int i = 0; i < g_nmovies; i++) {
        int dup = 0;
        for (int k = 0; k < ngenres; k++) dup |= !strcmp(genres[k], g_movies[i].genre);
        if (!dup && ngenres < 24) genres[ngenres++] = g_movies[i].genre;
    }
}
static void filter(void) {
    nitems = 0;
    for (int i = 0; i < g_nmovies; i++) {
        if (chip == 1 && !g_mylist[i]) continue;
        if (chip >= 2 && strcmp(genres[chip], g_movies[i].genre)) continue;
        items[nitems++] = i;
    }
}
static int focused_movie(void) {
    int idx = row * COLS + colx;
    return nitems ? items[clampi(idx, 0, nitems - 1)] : -1;
}

static void grid_enter(int arg) {
    build_genres();
    if (arg == -1) { filter(); return; }
    chip = 0; row = 0; colx = 0; on_chips = 0; vscroll = 0;
    filter();
}

static int grid_input(int key, int ch) {
    (void)ch;
    int rows = (nitems + COLS - 1) / COLS;
    if (on_chips) {
        switch (key) {
        case K_LEFT: if (chip > 0) { chip--; filter(); row = colx = 0; } return 1;
        case K_RIGHT: if (chip < ngenres - 1) { chip++; filter(); row = colx = 0; } return 1;
        case K_DOWN: case K_OK: if (nitems) on_chips = 0; return 1;
        case K_UP: app.nav_focused = 1; app.nav_index = TAB_MOVIES; return 1;
        }
        return 0;
    }
    switch (key) {
    case K_LEFT: if (colx > 0) colx--; return 1;
    case K_RIGHT: if (colx < COLS - 1 && row * COLS + colx + 1 < nitems) colx++; return 1;
    case K_UP: if (row > 0) row--; else on_chips = 1; return 1;
    case K_DOWN:
        if (row < rows - 1) { row++; if (row * COLS + colx >= nitems) colx = nitems - 1 - row * COLS; }
        return 1;
    case K_OK: if (focused_movie() >= 0) app_push(&scr_details, focused_movie()); return 1;
    case K_PLAY: if (focused_movie() >= 0) app_push(&scr_player, focused_movie()); return 1;
    }
    return 0;
}

static void grid_update(float dt) {
    int fm = on_chips ? -1 : row * COLS + colx;
    focus_update(&fa, MINI(nitems, 64), app.nav_focused ? -1 : fm, dt);
    focus_update(&fchips, ngenres, on_chips && !app.nav_focused ? chip : -1, dt);
    float target = on_chips ? 0 : fmaxf(0, row * 480.f - 60);
    vscroll = scroll_to(vscroll, target, dt);
    app.nav_scrim = smoothstep(20, 120, vscroll);
    int m = focused_movie();
    if (m != bg_cur && m >= 0) { bg_prev = bg_cur; bg_cur = m; bg_fade = 0; }
    bg_fade = fminf(1, bg_fade + dt / 0.6f);
    /* chips scroll */
    float cx = 0;
    for (int i = 0; i < chip; i++) cx += text_width(FONT_SEMIBOLD, 24, genres[i], 0) + 56 + 16;
    chip_scroll = scroll_to(chip_scroll, fmaxf(0, cx - 600), dt);
}

static void blur_bg(Gfx *g, int m, float a) {
    if (m < 0) return;
    const Texture *t = g_movies[m].blur;
    if (t && t->ready) g_image(g, t, 0, 0, SCREEN_W, SCREEN_H, 0, a);
}

static void grid_draw(Gfx *g) {
    g_rect(g, 0, 0, SCREEN_W, SCREEN_H, 0, HEX(0x241A4A));
    if (bg_prev >= 0 && bg_fade < 1) blur_bg(g, bg_prev, 1);
    blur_bg(g, bg_cur, bg_prev >= 0 ? ease_in_out_cubic(bg_fade) : 1);
    g_rect_vgrad(g, 0, 0, SCREEN_W, SCREEN_H, 0, rgba(255, 255, 255, 30), rgba(20, 8, 50, 110));
    g_push(g);
    g_translate(g, 0, -vscroll);
    g_text_shadow(g, FONT_DISPLAY, 58, SAFE_X, 150, "Movies", WHITE, ALIGN_LEFT, 0);
    char sub[64];
    snprintf(sub, sizeof sub, "%d titles  ·  all generated on this device", nitems);
    g_text(g, FONT_MEDIUM, 22, SAFE_X + 4, 228, sub, rgba(255, 255, 255, 190), ALIGN_LEFT, 0);
    /* genre chips */
    g_push(g);
    g_translate(g, -chip_scroll, 0);
    float cx = SAFE_X;
    for (int i = 0; i < ngenres; i++) {
        float w = text_width(FONT_SEMIBOLD, 24, genres[i], 0) + 56;
        float f = fchips.v[i];
        float x = cx, y = 280, h = 54, ww = w;
        ui_scale(&x, &y, &ww, &h, 1 + 0.06f * f);
        int sel = i == chip;
        g_rect(g, x, y, ww, h, h / 2, f > 0.01f ? col_lerp(rgba(255, 255, 255, sel ? 90 : 36), WHITE, f) : rgba(255, 255, 255, sel ? 90 : 36));
        if (!sel || f < 0.5f) g_stroke(g, x, y, ww, h, h / 2, 1.5f, rgba(255, 255, 255, 70));
        g_text(g, FONT_SEMIBOLD, 24, x + ww / 2, y + h / 2 - font_cap(FONT_SEMIBOLD, 24) / 2, genres[i], col_lerp(WHITE, rgba(20, 18, 36, 255), f), ALIGN_CENTER, 0);
        cx += w + 16;
    }
    g_pop(g);
    /* poster grid */
    const float PW = 240, PH = 360, GAPX = 48, ROWH = 480;
    float x0 = (SCREEN_W - (COLS * PW + (COLS - 1) * GAPX)) / 2, y0 = 380;
    if (!nitems) g_text(g, FONT_MEDIUM, 30, SCREEN_W / 2, y0 + 120, "Nothing here yet. Add films with \xE2\x80\x9CMy List\xE2\x80\x9D.", rgba(255, 255, 255, 200), ALIGN_CENTER, 0);
    int focus_i = -1;
    for (int pass = 0; pass < 2; pass++)
        for (int i = 0; i < nitems; i++) {
            float f = i < 64 ? fa.v[i] : 0;
            int is_focus = f > 0.02f;
            if (pass == 0 && is_focus) { focus_i = i; continue; }
            if (pass == 1 && !is_focus) continue;
            int r = i / COLS, c = i % COLS;
            float x = x0 + c * (PW + GAPX), y = y0 + r * ROWH, w = PW, h = PH;
            if (y - vscroll > SCREEN_H || y + h + 100 - vscroll < 0) continue;
            const Movie *m = &g_movies[items[i]];
            ui_scale(&x, &y, &w, &h, 1 + 0.1f * f * motion());
            ui_card_shadow(g, x, y, w, h, 16, f);
            ui_tex(g, m->poster, x, y, w, h, 16, 1);
            if (g_mylist[items[i]]) {
                g_circle(g, x + w - 26, y + 26, 16, accent());
                g_icon(g, IC_CHECK, x + w - 38, y + 14, 24, WHITE);
            }
            ui_focus_ring(g, x, y, w, h, 16, f);
            g_text_fit(g, f > 0.5f ? FONT_BOLD : FONT_SEMIBOLD, 22, x + w / 2, y + h + 22, m->title, rgba(255, 255, 255, (int)(190 + 65 * f)), w + 30, ALIGN_CENTER);
            if (f > 0.05f) {
                char meta[64];
                snprintf(meta, sizeof meta, "%s  ·  %s  ·  %.1f", m->genre, m->year, m->score);
                g_text(g, FONT_MEDIUM, 18, x + w / 2, y + h + 56, meta, rgba(255, 255, 255, (int)(200 * f)), ALIGN_CENTER, 0);
            }
        }
    (void)focus_i;
    g_pop(g);
}

Screen scr_movies = {"movies", TAB_MOVIES, grid_enter, grid_input, grid_update, grid_draw};

/* ======================= Details ======================= */
static int dm, drow, dcol, dmore[6];
static FocusAnim dfa[2];
static float dt_in;

static void details_enter(int arg) {
    if (arg >= 0) {
        dm = arg; drow = 0; dcol = 0; dt_in = 0;
        int n = 0;
        for (int i = 1; i < g_nmovies && n < 6; i++) {
            int j = (dm + i * 5) % g_nmovies;
            if (j != dm) dmore[n++] = j;
        }
    }
    film_prepare(dm);
}

static int details_input(int key, int ch) {
    (void)ch;
    switch (key) {
    case K_UP: drow = 0; dcol = MINI(dcol, 2); return 1;
    case K_DOWN: drow = 1; dcol = MINI(dcol, 5); return 1;
    case K_LEFT: if (dcol > 0) dcol--; return 1;
    case K_RIGHT: if (dcol < (drow ? 5 : 2)) dcol++; return 1;
    case K_PLAY: app_push(&scr_player, dm); return 1;
    case K_OK:
        if (drow == 0) {
            if (dcol == 0) app_push(&scr_player, dm);
            else if (dcol == 1) app_push(&scr_player, dm | 0x1000);
            else { g_mylist[dm] = !g_mylist[dm]; app_toast(g_mylist[dm] ? "Added to My List" : "Removed from My List", g_mylist[dm] ? IC_CHECK : IC_PLUS); }
        } else app_push(&scr_details, dmore[dcol]);
        return 1;
    }
    return 0;
}

static void details_update(float dt) {
    dt_in += dt;
    focus_update(&dfa[0], 3, drow == 0 ? dcol : -1, dt);
    focus_update(&dfa[1], 6, drow == 1 ? dcol : -1, dt);
}

static void details_draw(Gfx *g) {
    const Movie *m = &g_movies[dm];
    g_rect(g, 0, 0, SCREEN_W, SCREEN_H, 0, BLACK);
    const Texture *bd = m->backdrop && m->backdrop->ready ? m->backdrop : m->blur;
    ui_backdrop(g, bd, 1, 0);
    float in = ease_out_cubic(dt_in / 0.6f);
    g_push(g);
    g_opacity(g, in);
    g_translate(g, (1 - in) * 30 * motion(), 0);
    float x = SAFE_X;
    float lw = text_width(FONT_BOLD, 17, "KEEL ORIGINAL", 3) + 52;
    g_rect(g, x, 150, lw, 36, 18, rgba(255, 255, 255, 46));
    g_icon(g, IC_SPARKLE, x + 12, 157, 22, HEX(0xFFD166));
    g_text(g, FONT_BOLD, 17, x + 40, 168 - font_cap(FONT_BOLD, 17) / 2, "KEEL ORIGINAL", WHITE, ALIGN_LEFT, 3);
    float size = 96, tw = text_width(FONT_BLACK, size, m->title, 0);
    if (tw > 1100) size *= 1100 / tw;
    g_text_shadow(g, FONT_BLACK, size, x - 4, 214, m->title, WHITE, ALIGN_LEFT, 0);
    float y = 214 + font_cap(FONT_BLACK, size) + 28;
    g_text_shadow(g, FONT_LIGHT, 30, x, y, m->tagline, rgba(255, 255, 255, 230), ALIGN_LEFT, 0.5f);
    y += 62;
    char meta[128];
    snprintf(meta, sizeof meta, "%s  ·  %s  ·  %s", m->year, m->genre, m->runtime);
    float mw = g_text_shadow(g, FONT_SEMIBOLD, 26, x, y, meta, WHITE, ALIGN_LEFT, 0);
    float bx = x + mw + 22;
    const char *badges[4] = {m->rating, "4K", "HDR10", "5.1"};
    for (int i = 0; i < 4; i++) { ui_badge(g, bx, y - 5, badges[i], 1); bx += ui_badge_width(badges[i]) + 10; }
    ui_stars(g, bx + 14, y - 4, 26, m->score);
    y += 60;
    g_text_wrap(g, FONT_REGULAR, 27, x, y, 860, 40, 3, m->synopsis, rgba(255, 255, 255, 230));
    y += 140;
    char line[256];
    snprintf(line, sizeof line, "Director  %s", m->director);
    g_text(g, FONT_MEDIUM, 22, x, y, line, rgba(255, 255, 255, 200), ALIGN_LEFT, 0);
    snprintf(line, sizeof line, "Starring  %s, %s, %s", m->cast[0], m->cast[1], m->cast[2]);
    g_text_fit(g, FONT_MEDIUM, 22, x, y + 36, line, rgba(255, 255, 255, 200), 860, ALIGN_LEFT);
    g_pop(g);
    /* buttons */
    float bxx = x, by = 712;
    bxx += ui_button(g, bxx, by, 68, IC_PLAY, "Play", dfa[0].v[0], 1) + 22;
    bxx += ui_button(g, bxx, by, 68, IC_FILM, "Trailer", dfa[0].v[1], 0) + 22;
    ui_button(g, bxx, by, 68, g_mylist[dm] ? IC_CHECK : IC_PLUS, g_mylist[dm] ? "In My List" : "My List", dfa[0].v[2], 0);
    /* poster */
    float pk = ease_out_cubic((dt_in - 0.1f) / 0.7f);
    float pw = 330, ph = 495, px = SCREEN_W - SAFE_X - pw, py = 170 + (1 - pk) * 40;
    g_push(g);
    g_opacity(g, sat(pk));
    g_shadow(g, px, py + 30, pw, ph, 18, 70, rgba(0, 0, 0, 150));
    ui_tex(g, m->poster, px, py, pw, ph, 18, 1);
    g_stroke(g, px, py, pw, ph, 18, 1.5f, rgba(255, 255, 255, 90));
    g_pop(g);
    /* more like this */
    float gy = 820;
    ui_glass(g, 48, gy, SCREEN_W - 96, 300, 36, 0.8f);
    g_text(g, FONT_BOLD, 26, SAFE_X, gy + 28, "More Like This", WHITE, ALIGN_LEFT, 0);
    for (int i = 0; i < 6; i++) {
        const Movie *o = &g_movies[dmore[i]];
        float f = dfa[1].v[i];
        float tx = SAFE_X + i * 290, ty = gy + 76, w = 262, h = 147;
        ui_scale(&tx, &ty, &w, &h, 1 + 0.08f * f * motion());
        ui_card_shadow(g, tx, ty, w, h, 16, f);
        ui_tex(g, o->thumb && o->thumb->ready ? o->thumb : o->blur, tx, ty, w, h, 16, 1);
        g_rect_vgrad(g, tx, ty + h * 0.5f, w, h * 0.5f, 16, rgba(0, 0, 0, 0), rgba(0, 0, 0, 150));
        g_text_fit(g, FONT_BOLD, 20, tx + 16, ty + h - 34, o->title, WHITE, w - 32, ALIGN_LEFT);
        ui_focus_ring(g, tx, ty, w, h, 16, f);
    }
}

Screen scr_details = {"details", -1, details_enter, details_input, details_update, details_draw};

/* ======================= Player ======================= */
static int pm, trailer, paused;
static double ptime, pdur;
static float osd_t, seek_t, seek_dir, pause_anim;
static int was_music;

static double parse_runtime(const char *s) {
    int h = 0, m = 0;
    if (sscanf(s, "%dh %dm", &h, &m) < 1) return 6000;
    return h * 3600.0 + m * 60.0;
}

static void player_enter(int arg) {
    if (arg == -1) return;
    pm = arg & 0xFFF;
    trailer = (arg & 0x1000) != 0;
    paused = 0;
    osd_t = 0;
    seek_t = 9;
    pdur = trailer ? 150 : parse_runtime(g_movies[pm].runtime);
    ptime = 0;
    for (int i = 0; i < g_ncontinue; i++)
        if (g_continue[i].movie == pm && !trailer) ptime = pdur * g_continue[i].progress;
    film_prepare(pm);
    was_music = audio_playing() && !audio_paused();
    if (was_music) audio_toggle_pause();
}

static int player_input(int key, int ch) {
    (void)ch;
    osd_t = 0;
    switch (key) {
    case K_OK: case K_PLAY: paused = !paused; return 1;
    case K_LEFT: ptime = fmax(0, ptime - 10); seek_t = 0; seek_dir = -1; return 1;
    case K_RIGHT: ptime = fmin(pdur - 1, ptime + 10); seek_t = 0; seek_dir = 1; return 1;
    case K_UP: case K_DOWN: return 1;
    case K_CHAR: if (ch == 'c') { app.set.captions = !app.set.captions; return 1; } return 0;
    case K_BACK: case K_DEL:
        if (was_music && audio_paused()) audio_toggle_pause();
        app_back();
        return 1;
    }
    return 0;
}

static void player_update(float dt) {
    if (!paused && film_ready(pm)) ptime += dt;
    if (ptime >= pdur) { ptime = pdur; paused = 1; }
    osd_t += dt;
    seek_t += dt;
    pause_anim = approach(pause_anim, paused ? 1.f : 0.f, dt, 12);
}

static void fmt_time(char *b, int n, double s) {
    int t = (int)s;
    if (t >= 3600) snprintf(b, n, "%d:%02d:%02d", t / 3600, (t / 60) % 60, t % 60);
    else snprintf(b, n, "%d:%02d", t / 60, t % 60);
}

static const char *SUBS[] = {
    "We should have turned back at the ridge.", "If the signal is real, someone is still out there.",
    "Look at the light. It's never been this close.", "Nobody comes this far without a reason.",
    "Whatever happens next, we do it together.", "Hold on. Do you hear that?",
    "It was here all along. We just never looked up.", "Then let's finish what we started.",
};

static void player_draw(Gfx *g) {
    const Movie *m = &g_movies[pm];
    film_draw(g, pm, ptime, 0, 0, SCREEN_W, SCREEN_H, 1);
    if (!film_ready(pm)) {
        g_rect(g, 0, 0, SCREEN_W, SCREEN_H, 0, rgba(0, 0, 0, 90));
        ui_spinner(g, SCREEN_W / 2, SCREEN_H / 2 - 20, 34, 1);
        g_text(g, FONT_MEDIUM, 24, SCREEN_W / 2, SCREEN_H / 2 + 40, "Preparing your film\xE2\x80\xA6", rgba(255, 255, 255, 220), ALIGN_CENTER, 0);
    }
    /* captions */
    if (app.set.captions && film_ready(pm)) {
        int si = (int)(ptime / 6.0);
        float st = (float)fmod(ptime, 6.0);
        if (st > 0.6f && st < 5.0f) {
            const char *sline = SUBS[si % COUNT(SUBS)];
            float w = text_width(FONT_SEMIBOLD, 34, sline, 0);
            g_rect(g, SCREEN_W / 2 - w / 2 - 18, 880, w + 36, 58, 10, rgba(0, 0, 0, 150));
            g_text(g, FONT_SEMIBOLD, 34, SCREEN_W / 2, 893, sline, WHITE, ALIGN_CENTER, 0);
        }
    }
    /* pause veil */
    if (pause_anim > 0.01f) {
        g_rect(g, 0, 0, SCREEN_W, SCREEN_H, 0, rgba(0, 0, 0, (int)(70 * pause_anim)));
        float s = 120 * (0.8f + 0.2f * pause_anim);
        g_push(g);
        g_opacity(g, pause_anim);
        ui_glass_dark(g, SCREEN_W / 2 - s / 2 - 30, SCREEN_H / 2 - s / 2 - 30, s + 60, s + 60, (s + 60) / 2, 1);
        g_icon(g, IC_PAUSE, SCREEN_W / 2 - s * 0.35f, SCREEN_H / 2 - s * 0.35f, s * 0.7f, WHITE);
        g_pop(g);
    }
    /* seek bubble */
    if (seek_t < 0.8f) {
        float a = 1 - smoothstep(0.4f, 0.8f, seek_t);
        float cx = SCREEN_W / 2 + seek_dir * 420;
        g_push(g);
        g_opacity(g, a);
        ui_glass_dark(g, cx - 80, SCREEN_H / 2 - 50, 160, 100, 50, 1);
        g_icon(g, seek_dir < 0 ? IC_REW : IC_FFW, cx - 60, SCREEN_H / 2 - 20, 40, WHITE);
        g_text(g, FONT_BOLD, 28, cx + 2, SCREEN_H / 2 - font_cap(FONT_BOLD, 28) / 2, "10s", WHITE, ALIGN_LEFT, 0);
        g_pop(g);
    }
    /* OSD */
    float osd = paused ? 1.f : 1.f - smoothstep(3.5f, 4.2f, osd_t);
    if (osd > 0.01f) {
        g_push(g);
        g_opacity(g, osd);
        g_rect_vgrad(g, 0, 0, SCREEN_W, 260, 0, rgba(0, 0, 0, 170), rgba(0, 0, 0, 0));
        g_rect_vgrad(g, 0, SCREEN_H - 340, SCREEN_W, 340, 0, rgba(0, 0, 0, 0), rgba(0, 0, 0, 200));
        g_icon(g, IC_BACK, SAFE_X, 70, 34, rgba(255, 255, 255, 220));
        g_text(g, FONT_BOLD, 36, SAFE_X + 56, 68, m->title, WHITE, ALIGN_LEFT, 0);
        g_text(g, FONT_MEDIUM, 22, SAFE_X + 58, 118, trailer ? "Official Trailer" : m->tagline, rgba(255, 255, 255, 200), ALIGN_LEFT, 0);
        char clk[16];
        app_clock_str(clk, sizeof clk);
        g_text(g, FONT_SEMIBOLD, 30, SCREEN_W - SAFE_X, 70, clk, WHITE, ALIGN_RIGHT, 0);
        float py = SCREEN_H - 190, pw = SCREEN_W - 2 * SAFE_X;
        ui_glass_dark(g, SAFE_X - 30, py - 50, pw + 60, 190, 40, 0.9f);
        float p = (float)(ptime / pdur);
        g_rect(g, SAFE_X, py, pw, 8, 4, rgba(255, 255, 255, 60));
        g_rect(g, SAFE_X, py, pw * fminf(1.f, p + 0.08f), 8, 4, rgba(255, 255, 255, 60));
        g_rect(g, SAFE_X, py, pw * p, 8, 4, accent());
        g_shadow(g, SAFE_X + pw * p - 13, py - 9, 26, 26, 13, 10, rgba(0, 0, 0, 90));
        g_circle(g, SAFE_X + pw * p, py + 4, 13, WHITE);
        char a[32], b[32];
        fmt_time(a, sizeof a, ptime);
        fmt_time(b, sizeof b, pdur - ptime);
        g_text(g, FONT_SEMIBOLD, 22, SAFE_X, py + 30, a, WHITE, ALIGN_LEFT, 0);
        char rb[40];
        snprintf(rb, sizeof rb, "-%s", b);
        g_text(g, FONT_SEMIBOLD, 22, SAFE_X + pw, py + 30, rb, WHITE, ALIGN_RIGHT, 0);
        float cy = py + 50;
        g_icon(g, IC_REW, SCREEN_W / 2 - 150, cy + 6, 44, WHITE);
        g_circle(g, SCREEN_W / 2, cy + 28, 40, WHITE);
        g_icon(g, paused ? IC_PLAY : IC_PAUSE, SCREEN_W / 2 - 20, cy + 8, 40, rgba(20, 18, 36, 255));
        g_icon(g, IC_FFW, SCREEN_W / 2 + 106, cy + 6, 44, WHITE);
        g_icon(g, IC_CC, SCREEN_W - SAFE_X - 110, cy + 10, 38, app.set.captions ? WHITE : rgba(255, 255, 255, 140));
        g_icon(g, IC_VOLUME, SCREEN_W - SAFE_X - 40, cy + 10, 38, WHITE);
        g_text(g, FONT_MEDIUM, 20, SAFE_X, cy + 18, "4K  HDR10  ·  Dolby 5.1", rgba(255, 255, 255, 170), ALIGN_LEFT, 0);
        g_pop(g);
    }
}

Screen scr_player = {"player", -1, player_enter, player_input, player_update, player_draw};
