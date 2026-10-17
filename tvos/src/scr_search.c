/* KEEL OS — Search: on-screen keyboard with live results across movies,
 * channels, music and apps. Physical keyboards type directly too. */
#include "audio.h"
#include "ui.h"

#define KCOLS 6
static const char KEYS[] = "abcdefghijklmnopqrstuvwxyz1234567890";
static char query[64];
static int zone, kr, kc, rr, rc;
static float caret_t;
static FocusAnim fkeys, fres[4];
static float rscroll[4];

enum { R_MOVIE, R_CHANNEL, R_ALBUM, R_APP };
typedef struct { int type, n, idx[32]; const char *title; } ResRow;
static ResRow rows[4];
static int nrows;

static const char *SUGGEST[] = {"Sci-Fi", "Northern", "Ocean", "Synth", "Family", "Mountain", "News", "Piano"};

static int has(const char *hay, const char *needle) {
    if (!*needle) return 1;
    for (; *hay; hay++) {
        int i = 0;
        while (needle[i] && hay[i] && (hay[i] | 32) == (needle[i] | 32)) i++;
        if (!needle[i]) return 1;
    }
    return 0;
}

static void search(void) {
    nrows = 0;
    ResRow *r = &rows[nrows];
    r->type = R_MOVIE; r->n = 0; r->title = query[0] ? "Movies" : "Trending Now";
    for (int i = 0; i < g_nmovies && r->n < 32; i++) {
        const Movie *m = &g_movies[i];
        if (!query[0] ? (i % 2 == 0) : (has(m->title, query) || has(m->genre, query) || has(m->director, query) || has(m->cast[0], query) || has(m->cast[1], query) || has(m->synopsis, query)))
            r->idx[r->n++] = i;
    }
    if (r->n) nrows++;
    if (!query[0]) return;
    r = &rows[nrows]; r->type = R_CHANNEL; r->n = 0; r->title = "Live TV";
    for (int i = 0; i < g_nchannels; i++) {
        int hit = has(g_channels[i].name, query);
        for (int k = 0; k < 8 && !hit; k++) hit = has(g_channels[i].shows[k], query);
        if (hit) r->idx[r->n++] = i;
    }
    if (r->n) nrows++;
    r = &rows[nrows]; r->type = R_ALBUM; r->n = 0; r->title = "Music";
    for (int i = 0; i < g_nalbums; i++) {
        const Album *a = &g_albums[i];
        int hit = has(a->title, query) || has(a->artist, query) || has(a->genre, query);
        for (int k = 0; k < 5 && !hit; k++) hit = has(a->tracks[k], query);
        if (hit) r->idx[r->n++] = i;
    }
    if (r->n) nrows++;
    r = &rows[nrows]; r->type = R_APP; r->n = 0; r->title = "Apps";
    for (int i = 0; i < g_napps; i++)
        if (has(g_apps[i].name, query) || has(g_apps[i].tagline, query)) r->idx[r->n++] = i;
    if (r->n) nrows++;
}

static void enter(int arg) {
    if (arg == -1) { search(); return; }
    zone = 0; kr = kc = 0;
    search();
}

static void type_char(int ch) {
    int n = (int)strlen(query);
    if (n < 40) { query[n] = (char)ch; query[n + 1] = 0; }
    rr = rc = 0;
    search();
}
static void backspace(void) {
    int n = (int)strlen(query);
    if (n) query[n - 1] = 0;
    search();
}

static int krows(void) { return (int)(sizeof KEYS - 1) / KCOLS + 1; }

static void open_result(void) {
    if (rr >= nrows) return;
    ResRow *r = &rows[rr];
    int i = r->idx[clampi(rc, 0, r->n - 1)];
    switch (r->type) {
    case R_MOVIE: app_push(&scr_details, i); break;
    case R_CHANNEL: app_push(&scr_live_full, i); break;
    case R_ALBUM: audio_play(i, 0); app_push(&scr_nowplaying, 0); break;
    case R_APP: if (g_apps[i].target >= 0) app_tab(g_apps[i].target); else app_push(&scr_appsplash, i); break;
    }
}

static int input(int key, int ch) {
    if (key == K_CHAR) {
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == ' ' || ch == '-') { type_char(ch); return 1; }
        return 0;
    }
    if (key == K_DEL) { if (query[0]) { backspace(); return 1; } return 0; }
    if (zone == 0) {
        int last = kr == krows() - 1;
        int ncol = last ? 3 : KCOLS;
        switch (key) {
        case K_UP: if (kr > 0) { kr--; if (last) kc = kc * 2; } else { app.nav_focused = 1; app.nav_index = TAB_SEARCH; } return 1;
        case K_DOWN: if (kr < krows() - 1) { kr++; if (kr == krows() - 1) kc = MINI(kc / 2, 2); } return 1;
        case K_LEFT: if (kc > 0) kc--; return 1;
        case K_RIGHT:
            if (kc < ncol - 1) kc++;
            else if (nrows) { zone = 1; rr = MINI(rr, nrows - 1); rc = 0; }
            return 1;
        case K_OK:
            if (last) { if (kc == 0) type_char(' '); else if (kc == 1) backspace(); else { query[0] = 0; search(); } }
            else type_char(KEYS[kr * KCOLS + kc]);
            return 1;
        }
        return 0;
    }
    switch (key) {
    case K_UP: if (rr > 0) { rr--; rc = MINI(rc, rows[rr].n - 1); } else { app.nav_focused = 1; app.nav_index = TAB_SEARCH; } return 1;
    case K_DOWN: if (rr < nrows - 1) { rr++; rc = MINI(rc, rows[rr].n - 1); } return 1;
    case K_LEFT: if (rc > 0) rc--; else zone = 0; return 1;
    case K_RIGHT: if (rc < rows[rr].n - 1) rc++; return 1;
    case K_OK: open_result(); return 1;
    }
    return 0;
}

static void update(float dt) {
    caret_t += dt;
    int kf = zone == 0 && !app.nav_focused ? kr * KCOLS + kc : -1;
    focus_update(&fkeys, 40, kf, dt);
    for (int r = 0; r < 4; r++) {
        focus_update(&fres[r], r < nrows ? MINI(rows[r].n, 64) : 0, zone == 1 && r == rr && !app.nav_focused ? rc : -1, dt);
        float iw = r < nrows && rows[r].type == R_MOVIE ? 196 : 236;
        float target = r == rr ? fmaxf(0, rc * iw - iw * 2) : rscroll[r];
        rscroll[r] = scroll_to(rscroll[r], target, dt);
    }
}

static void draw(Gfx *g) {
    ui_wallpaper(g, 1);
    g_rect(g, 0, 0, SCREEN_W, SCREEN_H, 0, rgba(20, 10, 50, 60));
    /* keyboard panel */
    float px = 48, py = 140, pw = 660, ph = 912;
    ui_glass(g, px, py, pw, ph, 36, 1);
    float qx = px + 40, qy = py + 40, qw = pw - 80;
    g_rect(g, qx, qy, qw, 96, 24, rgba(255, 255, 255, 60));
    g_stroke(g, qx, qy, qw, 96, 24, 1.5f, rgba(255, 255, 255, 120));
    g_icon(g, IC_SEARCH, qx + 26, qy + 30, 36, WHITE);
    float tw = 0;
    if (query[0]) tw = g_text_fit(g, FONT_SEMIBOLD, 40, qx + 80, qy + 48 - font_cap(FONT_SEMIBOLD, 40) / 2, query, WHITE, qw - 110, ALIGN_LEFT);
    else g_text(g, FONT_MEDIUM, 34, qx + 80, qy + 48 - font_cap(FONT_MEDIUM, 34) / 2, "Movies, shows, music, apps", rgba(255, 255, 255, 140), ALIGN_LEFT, 0);
    if (fmodf(caret_t, 1.f) < 0.55f) g_rect(g, qx + 80 + tw + 4, qy + 24, 3, 48, 1.5f, accent());
    float kx = qx, ky = qy + 130, kw = 92, kh = 84, gap = 12;
    int nk = (int)sizeof KEYS - 1;
    for (int i = 0; i < nk; i++) {
        float f = fkeys.v[i];
        float x = kx + (i % KCOLS) * (kw + gap), y = ky + (i / KCOLS) * (kh + gap), w = kw, h = kh;
        ui_scale(&x, &y, &w, &h, 1 + 0.1f * f);
        if (f > 0.02f) g_shadow(g, x, y + 6, w, h, 18, 18, rgba(0, 0, 0, (int)(70 * f)));
        g_rect(g, x, y, w, h, 18, col_lerp(rgba(255, 255, 255, 34), WHITE, f));
        char s[2] = {(char)(KEYS[i] - (KEYS[i] >= 'a' ? 32 : 0)), 0};
        g_text(g, FONT_SEMIBOLD, 34, x + w / 2, y + h / 2 - font_cap(FONT_SEMIBOLD, 34) / 2, s, col_lerp(WHITE, rgba(20, 18, 36, 255), f), ALIGN_CENTER, 0);
    }
    float by = ky + 6 * (kh + gap);
    float bw3 = (6 * kw + 5 * gap - 2 * gap) / 3;
    const char *blab[3] = {"Space", "Delete", "Clear"};
    const int bicon[3] = {IC_SPACE, IC_DELETE, IC_POWER};
    for (int i = 0; i < 3; i++) {
        float f = (zone == 0 && kr == krows() - 1 && kc == i && !app.nav_focused) ? 1.f : 0.f;
        float x = kx + i * (bw3 + gap), y = by, w = bw3, h = kh;
        ui_scale(&x, &y, &w, &h, 1 + 0.06f * f);
        g_rect(g, x, y, w, h, 18, col_lerp(rgba(255, 255, 255, 34), WHITE, f));
        uint32_t c = col_lerp(WHITE, rgba(20, 18, 36, 255), f);
        float lw = text_width(FONT_SEMIBOLD, 22, blab[i], 0), iw = i < 2 ? 30 + 10 : 0;
        float sx = x + (w - lw - iw) / 2;
        if (i < 2) g_icon(g, bicon[i], sx, y + h / 2 - 15, 30, c);
        g_text(g, FONT_SEMIBOLD, 22, sx + iw, y + h / 2 - font_cap(FONT_SEMIBOLD, 22) / 2, blab[i], c, ALIGN_LEFT, 0);
    }
    g_text(g, FONT_MEDIUM, 19, px + pw / 2, py + ph - 48, "Tip: type on any keyboard", rgba(255, 255, 255, 150), ALIGN_CENTER, 0);
    /* results */
    float rx = 760, ry = 150;
    if (!query[0]) {
        g_text_shadow(g, FONT_BOLD, 30, rx, ry + 10, "Popular Searches", WHITE, ALIGN_LEFT, 0);
        float cx = rx;
        for (int i = 0; i < COUNT(SUGGEST); i++) {
            float w = text_width(FONT_SEMIBOLD, 22, SUGGEST[i], 0) + 70;
            g_rect(g, cx, ry + 64, w, 50, 25, rgba(255, 255, 255, 40));
            g_icon(g, IC_SEARCH, cx + 16, ry + 78, 22, rgba(255, 255, 255, 180));
            g_text(g, FONT_SEMIBOLD, 22, cx + 46, ry + 89 - font_cap(FONT_SEMIBOLD, 22) / 2, SUGGEST[i], WHITE, ALIGN_LEFT, 0);
            cx += w + 14;
        }
        ry += 150;
    } else {
        char hdr[96];
        int total = 0;
        for (int r = 0; r < nrows; r++) total += rows[r].n;
        if (total) snprintf(hdr, sizeof hdr, "%d results for \xE2\x80\x9C%s\xE2\x80\x9D", total, query);
        else snprintf(hdr, sizeof hdr, "No results for \xE2\x80\x9C%s\xE2\x80\x9D", query);
        g_text_shadow(g, FONT_BOLD, 32, rx, ry + 10, hdr, WHITE, ALIGN_LEFT, 0);
        ry += 80;
    }
    g_push(g);
    g_clip(g, rx - 30, 130, SCREEN_W - rx + 30, SCREEN_H - 130);
    for (int r = 0; r < nrows; r++) {
        ResRow *row = &rows[r];
        g_text(g, FONT_SEMIBOLD, 26, rx, ry, row->title, rgba(255, 255, 255, 230), ALIGN_LEFT, 0);
        float iy = ry + 50;
        float iw = row->type == R_MOVIE ? 176 : 212, ih = row->type == R_MOVIE ? 264 : (row->type == R_CHANNEL ? 127 : 212);
        g_push(g);
        g_translate(g, -rscroll[r], 0);
        for (int i = 0; i < row->n; i++) {
            float f = i < 64 ? fres[r].v[i] : 0;
            float x = rx + i * (iw + 20), y = iy, w = iw, h = ih;
            if (x - rscroll[r] > SCREEN_W + 50) break;
            ui_scale(&x, &y, &w, &h, 1 + 0.08f * f);
            ui_card_shadow(g, x, y, w, h, 16, f);
            int k = row->idx[i];
            const Texture *t = row->type == R_MOVIE ? g_movies[k].poster : row->type == R_CHANNEL ? g_channels[k].logo : row->type == R_ALBUM ? g_albums[k].cover : g_apps[k].tile;
            ui_tex(g, t, x, y, w, h, row->type == R_APP ? 40 : 16, 1);
            ui_focus_ring(g, x, y, w, h, row->type == R_APP ? 40 : 16, f);
            if (row->type != R_MOVIE) {
                const char *nm = row->type == R_CHANNEL ? g_channels[k].name : row->type == R_ALBUM ? g_albums[k].title : g_apps[k].name;
                g_text_fit(g, FONT_SEMIBOLD, 19, x + w / 2, y + h + 16, nm, rgba(255, 255, 255, (int)(180 + 75 * f)), w + 10, ALIGN_CENTER);
            }
        }
        g_pop(g);
        ry = iy + ih + (row->type == R_MOVIE ? 50 : 70);
    }
    g_pop(g);
}

Screen scr_search = {"search", TAB_SEARCH, enter, input, update, draw};
