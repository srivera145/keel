/* KEEL OS — Music: album shelf, track list, Now Playing and Visualizer. */
#include "audio.h"
#include "ui.h"

static int alb, mrow, mbtn, mtrack;
static FocusAnim fshelf, fbtn, ftrack;
static float shelf_scroll;
static int bg_a = -1, bg_b = -1;
static float bg_fade = 1, t_in;

static void track_len(char *b, int n, int album, int track) {
    int s = (int)audio_track_length(album, track);
    snprintf(b, n, "%d:%02d", s / 60, s % 60);
}

static void lib_enter(int arg) {
    if (arg == -1) return;
    t_in = 0;
    mrow = 0;
    if (audio_playing() && audio_album() >= 0) alb = audio_album();
}

static int lib_input(int key, int ch) {
    (void)ch;
    switch (key) {
    case K_UP:
        if (mrow == 0) { app.nav_focused = 1; app.nav_index = TAB_MUSIC; }
        else if (mrow == 2 && mtrack > 0) mtrack--;
        else mrow--;
        return 1;
    case K_DOWN:
        if (mrow == 0) mrow = 1;
        else if (mrow == 1) { mrow = 2; mtrack = 0; }
        else if (mtrack < 4) mtrack++;
        return 1;
    case K_LEFT:
        if (mrow == 0 && alb > 0) alb--;
        else if (mrow == 1 && mbtn > 0) mbtn--;
        return 1;
    case K_RIGHT:
        if (mrow == 0 && alb < g_nalbums - 1) alb++;
        else if (mrow == 1 && mbtn < 1) mbtn++;
        return 1;
    case K_OK:
        if (mrow == 0) audio_play(alb, 0);
        else if (mrow == 1) audio_play(alb, mbtn ? (int)(hash32((uint32_t)(app.time * 1000)) % 5) : 0);
        else audio_play(alb, mtrack);
        app_push(&scr_nowplaying, 0);
        return 1;
    }
    return 0;
}

static void lib_update(float dt) {
    t_in += dt;
    focus_update(&fshelf, g_nalbums, mrow == 0 && !app.nav_focused ? alb : -1, dt);
    focus_update(&fbtn, 2, mrow == 1 ? mbtn : -1, dt);
    focus_update(&ftrack, 5, mrow == 2 ? mtrack : -1, dt);
    float target = fmaxf(0, fminf(alb * 360.f - 360.f, g_nalbums * 360.f - (SCREEN_W - 2 * SAFE_X) + 20));
    shelf_scroll = scroll_to(shelf_scroll, target, dt);
    if (alb != bg_b) { bg_a = bg_b; bg_b = alb; bg_fade = 0; }
    bg_fade = fminf(1, bg_fade + dt / 0.6f);
}

static void cover_bg(Gfx *g, int a, float alpha) {
    if (a < 0) return;
    const Texture *t = g_albums[a].cover_blur;
    if (t && t->ready) g_image_src(g, t, 10, 10, 140, 79, 0, 0, SCREEN_W, SCREEN_H, 0, alpha);
}

static void eq_bars(Gfx *g, float x, float y, float h, uint32_t c) {
    for (int i = 0; i < 3; i++) {
        float v = audio_paused() ? 0.3f : 0.35f + 0.65f * fabsf(sinf((float)app.time * (5 + i * 2.3f) + i));
        g_rect(g, x + i * 9, y + h * (1 - v), 6, h * v, 3, c);
    }
}

static void lib_draw(Gfx *g) {
    g_rect(g, 0, 0, SCREEN_W, SCREEN_H, 0, HEX(0x2A1E5A));
    if (bg_a >= 0 && bg_fade < 1) cover_bg(g, bg_a, 1);
    cover_bg(g, bg_b, bg_a >= 0 ? ease_in_out_cubic(bg_fade) : 1);
    g_rect_vgrad(g, 0, 0, SCREEN_W, SCREEN_H, 0, rgba(20, 10, 40, 40), rgba(20, 10, 40, 140));
    g_text_shadow(g, FONT_DISPLAY, 58, SAFE_X, 150, "Music", WHITE, ALIGN_LEFT, 0);
    g_text(g, FONT_MEDIUM, 22, SAFE_X + 4, 228, "8 albums  ·  composed and played live by KEEL's synthesizer", rgba(255, 255, 255, 200), ALIGN_LEFT, 0);
    /* shelf */
    g_push(g);
    g_translate(g, -shelf_scroll, 0);
    for (int i = 0; i < g_nalbums; i++) {
        float f = fshelf.v[i];
        float x = SAFE_X + i * 360, y = 290, w = 320, h = 320;
        if (x - shelf_scroll > SCREEN_W || x + w - shelf_scroll < 0) continue;
        ui_scale(&x, &y, &w, &h, 1 + 0.08f * f * motion());
        ui_card_shadow(g, x, y, w, h, 20, f);
        ui_tex(g, g_albums[i].cover, x, y, w, h, 20, 1);
        ui_focus_ring(g, x, y, w, h, 20, f);
        if (audio_playing() && audio_album() == i) {
            g_rect(g, x + w - 62, y + 16, 46, 40, 12, rgba(0, 0, 0, 120));
            eq_bars(g, x + w - 52, y + 24, 24, WHITE);
        }
    }
    g_pop(g);
    /* detail panel */
    const Album *a = &g_albums[alb];
    float px = 48, py = 680, pw = SCREEN_W - 96, ph = 372;
    ui_glass(g, px, py, pw, ph, 36, 0.85f);
    g_text_fit(g, FONT_BLACK, 50, SAFE_X, py + 44, a->title, WHITE, 620, ALIGN_LEFT);
    g_text(g, FONT_SEMIBOLD, 28, SAFE_X, py + 116, a->artist, rgba(255, 255, 255, 230), ALIGN_LEFT, 0);
    char meta[96];
    snprintf(meta, sizeof meta, "%s  ·  %d BPM  ·  5 songs", a->genre, a->bpm);
    g_text(g, FONT_MEDIUM, 22, SAFE_X, py + 162, meta, rgba(255, 255, 255, 180), ALIGN_LEFT, 0);
    float bx = SAFE_X;
    bx += ui_button(g, bx, py + 236, 64, IC_PLAY, "Play", fbtn.v[0], 1) + 20;
    ui_button(g, bx, py + 236, 64, IC_SHUFFLE, "Shuffle", fbtn.v[1], 0);
    float lx = 760, lw = pw - (lx - px) - 40;
    for (int i = 0; i < 5; i++) {
        float f = ftrack.v[i];
        float ty = py + 30 + i * 64;
        int playing = audio_playing() && audio_album() == alb && audio_track() == i;
        if (f > 0.02f) {
            g_shadow(g, lx, ty + 4, lw, 56, 14, 16, rgba(0, 0, 0, (int)(60 * f)));
            g_rect(g, lx, ty, lw, 56, 14, rgba(255, 255, 255, (int)(255 * f)));
        }
        uint32_t c = col_lerp(WHITE, rgba(20, 18, 36, 255), f);
        if (playing) eq_bars(g, lx + 22, ty + 16, 24, f > 0.5f ? rgba(20, 18, 36, 255) : accent());
        else {
            char n[8];
            snprintf(n, sizeof n, "%d", i + 1);
            g_text(g, FONT_SEMIBOLD, 22, lx + 32, ty + 28 - font_cap(FONT_SEMIBOLD, 22) / 2, n, col_alpha(c, 0.7f), ALIGN_CENTER, 0);
        }
        g_text(g, FONT_SEMIBOLD, 24, lx + 72, ty + 28 - font_cap(FONT_SEMIBOLD, 24) / 2, a->tracks[i], c, ALIGN_LEFT, 0);
        char d[16];
        track_len(d, sizeof d, alb, i);
        g_text(g, FONT_MEDIUM, 22, lx + lw - 24, ty + 28 - font_cap(FONT_MEDIUM, 22) / 2, d, col_alpha(c, 0.7f), ALIGN_RIGHT, 0);
    }
}

Screen scr_music = {"music", TAB_MUSIC, lib_enter, lib_input, lib_update, lib_draw};

/* ======================= Now Playing / Visualizer ======================= */
static int viz, ctl = 2, ctl_row;
static float viz_t, osd_t, vfade;
static FocusAnim fctl;
static float spec[96], wave[512];
static float px_[160], py_[160], pv_[160];
static int shuffle, repeat;

static void np_enter(int arg) {
    if (arg == -1) return;
    viz = 0; vfade = 0; ctl = 2; ctl_row = 0;
    for (int i = 0; i < 160; i++) { px_[i] = 0; py_[i] = 0; pv_[i] = hashf(i * 31) * 2; }
}

static int np_input(int key, int ch) {
    (void)ch;
    osd_t = 0;
    if (viz) {
        switch (key) {
        case K_BACK: case K_DEL: viz = 0; return 1;
        case K_OK: case K_PLAY: audio_toggle_pause(); return 1;
        case K_LEFT: audio_prev(); return 1;
        case K_RIGHT: audio_next(); return 1;
        }
        return 1;
    }
    switch (key) {
    case K_LEFT: if (ctl_row == 0 && ctl > 0) ctl--; return 1;
    case K_RIGHT: if (ctl_row == 0 && ctl < 4) ctl++; return 1;
    case K_DOWN: ctl_row = 1; return 1;
    case K_UP: ctl_row = 0; return 1;
    case K_PLAY: audio_toggle_pause(); return 1;
    case K_CHAR: if (ch == 'v') { viz = 1; return 1; } return 0;
    case K_OK:
        if (ctl_row == 1) { viz = 1; viz_t = 0; return 1; }
        switch (ctl) {
        case 0: shuffle = !shuffle; app_toast(shuffle ? "Shuffle on" : "Shuffle off", IC_SHUFFLE); break;
        case 1: audio_prev(); break;
        case 2: audio_toggle_pause(); break;
        case 3: audio_next(); break;
        case 4: repeat = !repeat; app_toast(repeat ? "Repeat on" : "Repeat off", IC_REPEAT); break;
        }
        return 1;
    }
    return 0;
}

static void np_update(float dt) {
    osd_t += dt;
    viz_t += dt;
    vfade = approach(vfade, viz ? 1.f : 0.f, dt, 8);
    focus_update(&fctl, 6, ctl_row == 0 ? ctl : 5, dt);
    audio_spectrum(spec, 96);
    audio_waveform(wave, 512);
    float lvl = audio_level();
    for (int i = 0; i < 160; i++) {
        pv_[i] += dt * (0.08f + lvl * 2.2f) * (0.5f + hashf(i * 7));
        if (pv_[i] > 1.f) pv_[i] -= 1.f;
    }
}

static void np_bg(Gfx *g, const Album *a, float beat) {
    const Texture *cb = a->cover_blur;
    g_rect(g, 0, 0, SCREEN_W, SCREEN_H, 0, HEX(0x1A1238));
    if (cb && cb->ready) {
        float t = (float)app.time * 0.05f;
        float sx = 20 + 10 * sinf(t), sy = 20 + 10 * cosf(t * 0.8f);
        g_image_src(g, cb, sx, sy, 120, 67.5f, 0, 0, SCREEN_W, SCREEN_H, 0, 1);
    }
    g_rect(g, 0, 0, SCREEN_W, SCREEN_H, 0, rgba(10, 6, 24, (int)(70 - 30 * beat)));
}

static void draw_player(Gfx *g, const Album *a, float alpha, float beat) {
    g_push(g);
    g_opacity(g, alpha);
    float cs = 580 * (1 + 0.012f * beat * motion());
    float cx = 120 + 290 - cs / 2, cy = 190 + 290 - cs / 2;
    g_shadow(g, cx, cy + 40, cs, cs, 28, 80, rgba(0, 0, 0, 150));
    g_glow(g, cx + cs / 2, cy + cs / 2, cs * 0.7f, col_alpha(a->c0, 0.25f + 0.25f * beat));
    ui_tex(g, a->cover, cx, cy, cs, cs, 28, 1);
    g_stroke(g, cx, cy, cs, cs, 28, 1.5f, rgba(255, 255, 255, 80));
    float x = 820;
    g_text(g, FONT_BOLD, 20, x, 230, audio_paused() ? "PAUSED" : "NOW PLAYING", rgba(255, 255, 255, 190), ALIGN_LEFT, 4);
    int tr = audio_track();
    g_text_fit(g, FONT_BLACK, 76, x - 4, 280, a->tracks[tr % 5], WHITE, 1000, ALIGN_LEFT);
    char sub[128];
    snprintf(sub, sizeof sub, "%s  —  %s", a->artist, a->title);
    g_text(g, FONT_MEDIUM, 32, x, 390, sub, rgba(255, 255, 255, 220), ALIGN_LEFT, 0);
    /* progress */
    float pos = audio_position(), dur = audio_duration();
    float pw = 980;
    ui_progress(g, x, 480, pw, 8, pos / dur, WHITE);
    g_circle(g, x + pw * pos / dur, 484, 10, WHITE);
    char a1[16], b1[16];
    snprintf(a1, sizeof a1, "%d:%02d", (int)pos / 60, (int)pos % 60);
    snprintf(b1, sizeof b1, "-%d:%02d", (int)(dur - pos) / 60, (int)(dur - pos) % 60);
    g_text(g, FONT_SEMIBOLD, 22, x, 510, a1, rgba(255, 255, 255, 200), ALIGN_LEFT, 0);
    g_text(g, FONT_SEMIBOLD, 22, x + pw, 510, b1, rgba(255, 255, 255, 200), ALIGN_RIGHT, 0);
    /* transport */
    static const int icons[5] = {IC_SHUFFLE, IC_PREV, IC_PLAY, IC_NEXT, IC_REPEAT};
    float tx = x + pw / 2 - 2 * 150;
    for (int i = 0; i < 5; i++) {
        float f = fctl.v[i];
        float sz = i == 2 ? 112 : 84;
        float bx = tx + i * 150 - sz / 2, by = 610 - sz / 2;
        float sx = bx, sy = by, sw = sz, sh = sz;
        ui_scale(&sx, &sy, &sw, &sh, 1 + 0.1f * f);
        int on = (i == 0 && shuffle) || (i == 4 && repeat);
        if (i == 2) {
            g_shadow(g, sx, sy + 10, sw, sh, sw / 2, 30, rgba(0, 0, 0, 90));
            g_circle(g, sx + sw / 2, sy + sh / 2, sw / 2, WHITE);
            if (f > 0.02f) g_stroke(g, sx - 6, sy - 6, sw + 12, sh + 12, sw / 2 + 6, 3, rgba(255, 255, 255, (int)(255 * f)));
            g_icon(g, audio_paused() ? IC_PLAY : IC_PAUSE, sx + sw * 0.28f, sy + sh * 0.28f, sw * 0.44f, rgba(20, 18, 36, 255));
        } else {
            if (f > 0.02f) g_rect(g, sx, sy, sw, sh, sw / 2, rgba(255, 255, 255, (int)(60 + 60 * f)));
            g_icon(g, icons[i], sx + sw * 0.27f, sy + sh * 0.27f, sw * 0.46f, on ? accent() : WHITE);
        }
    }
    ui_button(g, x + pw / 2 - ui_button_width(60, IC_SPARKLE, "Visualizer") / 2, 720, 60, IC_SPARKLE, "Visualizer", fctl.v[5], 0);
    /* spectrum strip */
    float sx0 = 0, sy0 = SCREEN_H - 30, bw = SCREEN_W / 96.f;
    for (int i = 0; i < 96; i++) {
        float v = spec[i];
        float h = 20 + v * 230;
        uint32_t c = col_lerp(a->c0, a->c1, i / 95.f);
        g_rect(g, sx0 + i * bw + 3, sy0 - h, bw - 6, h, (bw - 6) / 2, col_alpha(c, 0.75f));
    }
    g_pop(g);
}

static void draw_viz(Gfx *g, const Album *a, float alpha, float beat) {
    g_push(g);
    g_opacity(g, alpha);
    g_rect(g, 0, 0, SCREEN_W, SCREEN_H, 0, rgba(6, 3, 18, 150));
    float cx = 960, cy = 520;
    float lvl = audio_level();
    float R = 230 + 26 * beat * motion();
    g_glow(g, cx, cy, 480 + 120 * beat, col_alpha(a->c0, 0.3f + 0.3f * beat));
    g_glow(g, cx - 420, cy + 220, 380, col_alpha(a->c1, 0.22f + 0.25f * lvl));
    g_glow(g, cx + 460, cy - 200, 360, col_alpha(a->c2, 0.2f + 0.25f * lvl));
    /* particles flying outward */
    for (int i = 0; i < 160; i++) {
        float ang = hashf(i * 13) * 2 * PI_F + (float)app.time * 0.05f;
        float d = R + pv_[i] * 900;
        float x = cx + cosf(ang) * d, y = cy + sinf(ang) * d * 0.9f;
        float a2 = (1 - pv_[i]) * 0.8f;
        g_circle(g, x, y, 2 + 3 * (1 - pv_[i]), col_alpha(col_lerp(WHITE, a->c0, hashf(i)), a2));
    }
    /* radial spectrum, mirrored */
    float rot = (float)app.time * 0.12f;
    for (int i = 0; i < 96; i++) {
        /* mirrored around the vertical axis: bass at the bottom, highs on top */
        int k = i < 48 ? i : 95 - i;
        float v = spec[4 + k * 3 / 2] * (0.75f + 0.25f * sinf((float)app.time * 2 + k));
        float ang = PI_F * 0.5f + (i < 48 ? 1.f : -1.f) * (k + 0.5f) * PI_F / 48 + 0 * rot;
        float len = 14 + v * 210;
        float c = cosf(ang), s = sinf(ang);
        uint32_t col = col_lerp(a->c0, a->c1, v);
        g_line(g, cx + c * (R + 14), cy + s * (R + 14), cx + c * (R + 14 + len), cy + s * (R + 14 + len), 9, col_alpha(col, 0.9f));
    }
    g_stroke(g, cx - R, cy - R, 2 * R, 2 * R, R, 3, rgba(255, 255, 255, 120));
    float cr = R - 24;
    if (a->cover && a->cover->ready) g_image_src(g, a->cover, 90, 40, 420, 420, cx - cr, cy - cr, 2 * cr, 2 * cr, cr, 1);
    /* waveform */
    float wy = 960;
    float lastx = 0, lasty = wy;
    for (int i = 0; i < 128; i++) {
        float x = i * SCREEN_W / 127.f;
        float y = wy + wave[i * 4] * 160;
        if (i) g_line(g, lastx, lasty, x, y, 3, col_alpha(a->c1, 0.85f));
        lastx = x; lasty = y;
    }
    float oa = 1 - smoothstep(3, 4, osd_t);
    if (oa > 0.01f) {
        g_push(g);
        g_opacity(g, oa);
        ui_glass_dark(g, SAFE_X - 24, 64, 640, 110, 28, 1);
        ui_tex(g, a->cover, SAFE_X - 4, 80, 78, 78, 12, 1);
        g_text_fit(g, FONT_BOLD, 28, SAFE_X + 94, 92, a->tracks[audio_track() % 5], WHITE, 500, ALIGN_LEFT);
        g_text(g, FONT_MEDIUM, 22, SAFE_X + 94, 134, a->artist, rgba(255, 255, 255, 200), ALIGN_LEFT, 0);
        g_pop(g);
    }
    g_pop(g);
}

static void np_draw(Gfx *g) {
    int ai = audio_album() >= 0 ? audio_album() : 0;
    const Album *a = &g_albums[ai];
    float beat = audio_beat();
    np_bg(g, a, beat);
    if (vfade < 0.99f) draw_player(g, a, 1 - vfade, beat);
    if (vfade > 0.01f) draw_viz(g, a, vfade, beat);
}

Screen scr_nowplaying = {"nowplaying", -1, np_enter, np_input, np_update, np_draw};
