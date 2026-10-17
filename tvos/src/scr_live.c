/* KEEL OS — Live TV: program guide with live preview, full-screen channels. */
#include "channels.h"
#include "ui.h"

static int ch, pcol;           /* focused channel, program column (0 = now) */
static float rscroll, hl_y, hl_x, hl_w;
static float t_in;
static int live_ch;
static float banner_t, switch_t;

static const char *DESC[] = {
    "Live coverage, analysis and the stories shaping tonight, from our newsroom.",
    "Breathtaking wildlife and wild places, filmed across the high country.",
    "Live football: City take on Harbor in a top-of-the-table clash.",
    "A journey to the edge of the observable universe and back.",
    "Your local and national forecast, updated live every half hour.",
    "Dive into vibrant reefs and the mysterious creatures of the deep.",
    "Nonstop synthwave and retro anthems under neon skies.",
    "Songs, shapes and giggles for little viewers.",
    "Back-to-back blockbusters from the KEEL Studios collection.",
};

static int now_min(void) { return app_wall_minutes(); }
static void fmt_hm(char *b, int n, int m) {
    m = ((m % 1440) + 1440) % 1440;
    int h = m / 60, mm = m % 60;
    if (app.set.clock24) snprintf(b, n, "%02d:%02d", h, mm);
    else snprintf(b, n, "%d:%02d", h % 12 == 0 ? 12 : h % 12, mm);
}
static void program_at_col(int c, int col, Program *p) {
    schedule_at(c, now_min(), p);
    for (int i = 0; i < col; i++) { Program n; schedule_next(c, p, &n); *p = n; }
}

static void enter(int arg) {
    if (arg == -1) return;
    t_in = 0;
    pcol = 0;
}

static int input(int key, int chr) {
    (void)chr;
    switch (key) {
    case K_UP:
        if (ch > 0) { ch--; pcol = 0; }
        else { app.nav_focused = 1; app.nav_index = TAB_LIVE; }
        return 1;
    case K_DOWN: if (ch < g_nchannels - 1) { ch++; pcol = 0; } return 1;
    case K_CHUP: if (ch > 0) ch--; return 1;
    case K_CHDN: if (ch < g_nchannels - 1) ch++; return 1;
    case K_LEFT: if (pcol > 0) pcol--; return 1;
    case K_RIGHT: if (pcol < 4) pcol++; return 1;
    case K_OK:
        if (pcol == 0) app_push(&scr_live_full, ch);
        else {
            Program p;
            program_at_col(ch, pcol, &p);
            char msg[128], tm[16];
            fmt_hm(tm, sizeof tm, p.start);
            snprintf(msg, sizeof msg, "Reminder set: %s at %s", p.title, tm);
            app_toast(msg, IC_BELL);
        }
        return 1;
    }
    return 0;
}

static void update(float dt) {
    t_in += dt;
    float target = fmaxf(0, fminf((float)(ch - 2), (float)(g_nchannels - 5))) * 76;
    rscroll = scroll_to(rscroll, target, dt);
}

static void draw(Gfx *g) {
    const Channel *c = &g_channels[ch];
    C3 cc = c3hex(c->color & 0xFFFFFF);
    g_rect_grad(g, 0, 0, SCREEN_W, SCREEN_H, 0, c3col(c3lerp(cc, c3hex(0x2B1E6E), 0.45f), 1), c3col(c3hex(0x120A2E), 1), 0, 0, SCREEN_W * 0.6f, SCREEN_H);
    g_glow(g, 1450, 360, 700, c3col(cc, 0.35f));
    /* live preview */
    float px = 1032, py = 150, pw = 792, ph = 445;
    g_shadow(g, px, py + 24, pw, ph, 26, 60, rgba(0, 0, 0, 140));
    g_layer_begin(g, px, py, pw, ph);
    channel_draw(g, ch, app.time, px, py, pw, ph);
    g_layer_end(g, 26, 1);
    g_stroke(g, px, py, pw, ph, 26, 2, rgba(255, 255, 255, 120));
    /* channel info */
    Program p;
    schedule_at(ch, now_min(), &p);
    float x = SAFE_X, y = 160;
    ui_tex(g, c->logo, x, y, 140, 84, 16, 1);
    char num[16];
    snprintf(num, sizeof num, "%d", c->number);
    g_text(g, FONT_BOLD, 28, x + 168, y + 14, num, rgba(255, 255, 255, 200), ALIGN_LEFT, 0);
    g_text(g, FONT_BOLD, 34, x + 168, y + 52, c->name, WHITE, ALIGN_LEFT, 0);
    g_rect(g, x, y + 120, 96, 36, 18, rgba(230, 40, 60, 240));
    g_circle(g, x + 20, y + 138, 6, rgba(255, 255, 255, (int)(160 + 95 * sinf((float)app.time * 4))));
    g_text(g, FONT_BOLD, 18, x + 34, y + 138 - font_cap(FONT_BOLD, 18) / 2, "LIVE", WHITE, ALIGN_LEFT, 2);
    g_text_shadow(g, FONT_BLACK, 56, x, y + 190, p.title, WHITE, ALIGN_LEFT, 0);
    char a[16], b[16], tr[48];
    fmt_hm(a, sizeof a, p.start);
    fmt_hm(b, sizeof b, p.start + p.dur);
    snprintf(tr, sizeof tr, "%s – %s", a, b);
    g_text(g, FONT_SEMIBOLD, 24, x, y + 276, tr, rgba(255, 255, 255, 220), ALIGN_LEFT, 0);
    float prog = (now_min() - p.start + (float)fmod(app.time, 60) / 60.f) / p.dur;
    ui_progress(g, x + 200, y + 284, 320, 7, prog, accent());
    g_text_wrap(g, FONT_REGULAR, 24, x, y + 322, 840, 34, 2, DESC[c->kind], rgba(255, 255, 255, 210));
    /* guide */
    float gx = 48, gy = 640, gw = SCREEN_W - 96, gh = 412;
    ui_glass(g, gx, gy, gw, gh, 36, 0.85f);
    float colw = 330, tlx = gx + colw, tlw = gw - colw - 30;
    int start = now_min() / 30 * 30;
    float ppm = tlw / 120.f;
    for (int i = 0; i < 4; i++) {
        char tm[16];
        fmt_hm(tm, sizeof tm, start + i * 30);
        g_text(g, FONT_SEMIBOLD, 20, tlx + i * 30 * ppm + 10, gy + 26, tm, rgba(255, 255, 255, 190), ALIGN_LEFT, 0);
        g_rect(g, tlx + i * 30 * ppm, gy + 20, 1.5f, 28, 0, rgba(255, 255, 255, 60));
    }
    float rows_y = gy + 64, rh = 76;
    g_push(g);
    g_clip(g, gx, rows_y - 4, gw, gh - 70);
    float hl_target_y = 0, hl_target_x = 0, hl_target_w = 0;
    for (int r = 0; r < g_nchannels; r++) {
        float ry = rows_y + r * rh - rscroll;
        if (ry > gy + gh || ry + rh < rows_y - 4) continue;
        const Channel *rc = &g_channels[r];
        int focus_row = r == ch;
        ui_tex(g, rc->logo, gx + 34, ry + 8, 100, 60, 12, 1);
        char n2[16];
        snprintf(n2, sizeof n2, "%d", rc->number);
        g_text(g, FONT_BOLD, 22, gx + 152, ry + 18, n2, rgba(255, 255, 255, focus_row ? 255 : 170), ALIGN_LEFT, 0);
        g_text_fit(g, FONT_SEMIBOLD, 20, gx + 152, ry + 46, rc->name, rgba(255, 255, 255, focus_row ? 230 : 150), colw - 170, ALIGN_LEFT);
        Program q;
        schedule_at(r, start, &q);
        for (int col = 0; col < 8 && q.start < start + 120; col++) {
            float bx0 = tlx + (q.start - start) * ppm, bx1 = tlx + (q.start + q.dur - start) * ppm;
            float cx0 = fmaxf(bx0, tlx), cx1 = fminf(bx1, tlx + tlw);
            Program cur;
            schedule_at(r, now_min(), &cur);
            int is_now = q.start == cur.start;
            int colidx = 0;
            { Program tmp = cur; while (tmp.start < q.start) { Program n3; schedule_next(r, &tmp, &n3); tmp = n3; colidx++; } }
            int focused = focus_row && colidx == pcol && !app.nav_focused;
            if (cx1 - cx0 > 8) {
                if (focused) { hl_target_y = ry + 6; hl_target_x = cx0 + 3; hl_target_w = cx1 - cx0 - 6; }
                else g_rect(g, cx0 + 3, ry + 6, cx1 - cx0 - 6, rh - 12, 14, is_now ? rgba(255, 255, 255, 48) : rgba(255, 255, 255, 22));
                if (!focused) g_text_fit(g, FONT_SEMIBOLD, 22, cx0 + 20, ry + 28, q.title, rgba(255, 255, 255, is_now ? 240 : 170), cx1 - cx0 - 40, ALIGN_LEFT);
            }
            Program n4;
            schedule_next(r, &q, &n4);
            q = n4;
        }
    }
    /* animated focus block */
    if (hl_target_w > 0) {
        if (hl_w <= 0) { hl_x = hl_target_x; hl_y = hl_target_y; hl_w = hl_target_w; }
        hl_x = approach(hl_x, hl_target_x, app.dt, 18);
        hl_y = approach(hl_y, hl_target_y, app.dt, 18);
        hl_w = approach(hl_w, hl_target_w, app.dt, 18);
        g_shadow(g, hl_x, hl_y + 6, hl_w, rh - 12, 14, 20, rgba(0, 0, 0, 80));
        g_rect(g, hl_x, hl_y, hl_w, rh - 12, 14, WHITE);
        Program fp;
        program_at_col(ch, pcol, &fp);
        g_text_fit(g, FONT_BOLD, 22, hl_x + 18, hl_y + 22, fp.title, rgba(20, 18, 36, 255), hl_w - 36, ALIGN_LEFT);
        char ta[16];
        fmt_hm(ta, sizeof ta, fp.start);
        if (hl_w > 200) g_text(g, FONT_MEDIUM, 17, hl_x + hl_w - 18, hl_y + 25, ta, rgba(20, 18, 36, 160), ALIGN_RIGHT, 0);
    }
    g_pop(g);
    /* now line */
    float nx = tlx + (now_min() + (float)fmod(app.time, 60) / 60.f - start) * ppm;
    g_rect(g, nx - 1.5f, gy + 52, 3, gh - 70, 1.5f, accent());
    g_circle(g, nx, gy + 52, 7, accent());
}

Screen scr_live = {"live", TAB_LIVE, enter, input, update, draw};

/* ======================= Full screen ======================= */
static void full_enter(int arg) {
    if (arg >= 0) live_ch = arg;
    banner_t = 0;
    switch_t = 9;
}

static int full_input(int key, int chr) {
    (void)chr;
    switch (key) {
    case K_UP: case K_CHUP: live_ch = (live_ch + g_nchannels - 1) % g_nchannels; banner_t = 0; switch_t = 0; ch = live_ch; return 1;
    case K_DOWN: case K_CHDN: live_ch = (live_ch + 1) % g_nchannels; banner_t = 0; switch_t = 0; ch = live_ch; return 1;
    case K_OK: banner_t = banner_t < 4 ? 9 : 0; return 1;
    case K_LEFT: case K_RIGHT: banner_t = 0; return 1;
    }
    return 0;
}

static void full_update(float dt) { banner_t += dt; switch_t += dt; }

static void full_draw(Gfx *g) {
    channel_draw(g, live_ch, app.time, 0, 0, SCREEN_W, SCREEN_H);
    if (switch_t < 0.45f) g_rect(g, 0, 0, SCREEN_W, SCREEN_H, 0, rgba(0, 0, 0, (int)(255 * (1 - switch_t / 0.45f))));
    float a = 1 - smoothstep(4.5f, 5.2f, banner_t);
    if (a <= 0.01f) return;
    const Channel *c = &g_channels[live_ch];
    Program p, n;
    schedule_at(live_ch, now_min(), &p);
    schedule_next(live_ch, &p, &n);
    float in = ease_out_cubic(banner_t / 0.4f);
    g_push(g);
    g_opacity(g, a);
    g_translate(g, 0, (1 - in) * 40);
    float bx = 72, by = SCREEN_H - 250, bw = SCREEN_W - 144, bh = 190;
    ui_glass(g, bx, by, bw, bh, 40, 1);
    ui_tex(g, c->logo, bx + 32, by + 40, 180, 108, 18, 1);
    char num[16];
    snprintf(num, sizeof num, "%d", c->number);
    g_text(g, FONT_BLACK, 44, bx + 250, by + 40, num, WHITE, ALIGN_LEFT, 0);
    g_text(g, FONT_SEMIBOLD, 24, bx + 250, by + 104, c->name, rgba(255, 255, 255, 220), ALIGN_LEFT, 0);
    g_text_fit(g, FONT_BOLD, 40, bx + 560, by + 40, p.title, WHITE, 760, ALIGN_LEFT);
    char a1[16], b1[16], tr[64];
    fmt_hm(a1, sizeof a1, p.start);
    fmt_hm(b1, sizeof b1, p.start + p.dur);
    snprintf(tr, sizeof tr, "%s – %s", a1, b1);
    g_text(g, FONT_MEDIUM, 22, bx + 560, by + 100, tr, rgba(255, 255, 255, 210), ALIGN_LEFT, 0);
    float prog = (now_min() - p.start + (float)fmod(app.time, 60) / 60.f) / p.dur;
    ui_progress(g, bx + 760, by + 108, 520, 7, prog, accent());
    char nx[96];
    snprintf(nx, sizeof nx, "Up next  ·  %s", n.title);
    g_text_fit(g, FONT_MEDIUM, 22, bx + 560, by + 140, nx, rgba(255, 255, 255, 180), 700, ALIGN_LEFT);
    g_icon(g, IC_CHEV_U, bx + bw - 150, by + 48, 34, rgba(255, 255, 255, 200));
    g_icon(g, IC_CHEV_D, bx + bw - 150, by + 104, 34, rgba(255, 255, 255, 200));
    g_text(g, FONT_SEMIBOLD, 20, bx + bw - 106, by + 86, "CH", rgba(255, 255, 255, 200), ALIGN_LEFT, 1);
    g_pop(g);
}

Screen scr_live_full = {"live_full", -1, full_enter, full_input, full_update, full_draw};
