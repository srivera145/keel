/* KEEL OS — live channel renderers. Each "broadcast" is animated in real
 * time from vector primitives and generated imagery, at any size (guide
 * preview or full screen). Content is authored in 1920x1080 units. */
#include "channels.h"
#include "font.h"
#include "icons.h"
#include "ui.h"

typedef struct { Gfx *g; float x, y, k; double t; } CC;
#define X(v) (c->x + (v) * c->k)
#define Y(v) (c->y + (v) * c->k)
#define S(v) ((v) * c->k)

static void ktext(CC *c, int font, float size, float x, float y, const char *s, uint32_t col, int align, float tr) {
    float sz = S(size);
    if (sz < 5) return;
    g_text(c->g, font, sz, X(x), Y(y), s, col, align, S(tr));
}

static void bug(CC *c, const Channel *ch, int live) {
    g_rect(c->g, X(1640), Y(52), S(200), S(56), S(12), rgba(0, 0, 0, 70));
    ktext(c, FONT_BLACK, 30, 1740, 66, ch->code, rgba(255, 255, 255, 220), ALIGN_CENTER, 2);
    if (live) {
        g_rect(c->g, X(80), Y(56), S(116), S(46), S(23), rgba(230, 40, 60, 230));
        g_circle(c->g, X(104), Y(79), S(7), rgba(255, 255, 255, (int)(160 + 95 * sinf((float)c->t * 4))));
        ktext(c, FONT_BOLD, 22, 120, 71, "LIVE", WHITE, ALIGN_LEFT, 2);
    }
}

static void ken_burns(CC *c, const Texture *t, float speed, float phase) {
    if (!t || !t->ready) return;
    float p = 0.5f + 0.5f * sinf((float)c->t * speed + phase);
    float z = 1.12f;
    float sw = t->w / z, sh = t->h / z;
    g_image_src(c->g, t, (t->w - sw) * p, (t->h - sh) * 0.5f, sw, sh, X(0), Y(0), S(1920), S(1080), 0, 1);
}

/* ---------------- News ---------------- */
static void ch_news(CC *c, const Channel *ch) {
    Gfx *g = c->g;
    float t = (float)c->t;
    g_rect_grad(g, X(0), Y(0), S(1920), S(1080), 0, HEX(0x0B1E5B), HEX(0x5A0E2E), X(0), Y(0), X(1920), Y(1080));
    for (int i = 0; i < 6; i++) {
        float ox = fmodf(t * 80 + i * 380, 2600) - 600;
        float q[] = {X(ox), Y(0), X(ox + 160), Y(0), X(ox - 340), Y(1080), X(ox - 500), Y(1080)};
        g_poly(g, q, 4, rgba(255, 255, 255, 10 + (i % 3) * 5));
    }
    /* globe */
    float gx = 1320, gy = 430, R = 300;
    g_glow(g, X(gx), Y(gy), S(R * 1.4f), rgba(80, 160, 255, 70));
    g_circle(g, X(gx), Y(gy), S(R), rgba(20, 70, 170, 255));
    g_circle(g, X(gx - 60), Y(gy - 70), S(R * 0.75f), rgba(60, 130, 230, 90));
    for (int m = 0; m < 8; m++) {
        float ph = t * 0.3f + m * PI_F / 8;
        float rx = R * cosf(ph);
        float px = 0, py = 0;
        for (int s = 0; s <= 16; s++) {
            float a = -PI_F / 2 + PI_F * s / 16;
            float qx = gx + rx * cosf(a), qy = gy + R * sinf(a);
            if (s) g_line(g, X(px), Y(py), X(qx), Y(qy), S(2), rgba(150, 210, 255, sinf(ph) > 0 ? 120 : 40));
            px = qx; py = qy;
        }
    }
    for (int l = -2; l <= 2; l++) {
        float yy = gy + l * R * 0.38f, hw = sqrtf(R * R - (yy - gy) * (yy - gy));
        g_line(g, X(gx - hw), Y(yy), X(gx + hw), Y(yy), S(2), rgba(150, 210, 255, 90));
    }
    for (int i = 0; i < 9; i++) {
        float a = hashf(i * 7 + 1) * 2 * PI_F + t * 0.3f, b = (hashf(i * 13 + 5) - 0.5f) * 2.2f;
        float px = gx + R * 0.95f * sinf(a) * cosf(b * 0.6f), py = gy + R * 0.9f * sinf(b * 0.6f);
        if (cosf(a) < 0) continue;
        float pulse = 0.5f + 0.5f * sinf(t * 3 + i);
        g_glow(g, X(px), Y(py), S(26), rgba(255, 210, 90, (int)(180 * pulse)));
        g_circle(g, X(px), Y(py), S(5), rgba(255, 240, 200, 255));
    }
    /* anchor desk + presenter */
    g_circle(g, X(520), Y(430), S(78), rgba(30, 24, 50, 255));
    g_rect(g, X(380), Y(510), S(280), S(330), S(110), rgba(30, 24, 50, 255));
    g_rect(g, X(470), Y(520), S(100), S(140), S(30), rgba(240, 240, 250, 255));
    g_rect(g, X(500), Y(520), S(40), S(150), S(10), rgba(200, 40, 60, 255));
    g_rect_vgrad(g, X(140), Y(640), S(820), S(240), S(24), rgba(255, 255, 255, 230), rgba(190, 200, 230, 255));
    ktext(c, FONT_BLACK, 56, 550, 720, "KEEL 24", rgba(20, 40, 110, 255), ALIGN_CENTER, 4);
    /* lower third */
    g_rect(g, X(0), Y(880), S(1920), S(110), 0, rgba(255, 255, 255, 240));
    g_rect(g, X(0), Y(880), S(330), S(110), 0, rgba(210, 30, 50, 255));
    ktext(c, FONT_BLACK, 40, 165, 917, "BREAKING", WHITE, ALIGN_CENTER, 2);
    static const char *heads[] = {"Record-breaking aurora lights up skies across the north", "Harbor city unveils car-free waterfront plan",
                                  "Deep-space listening post detects unexplained signal", "Global markets rally as tech stocks climb"};
    ktext(c, FONT_BOLD, 38, 370, 917, heads[((int)(t / 8)) % 4], rgba(20, 20, 40, 255), ALIGN_LEFT, 0);
    g_rect(g, X(0), Y(990), S(1920), S(70), 0, rgba(12, 20, 60, 240));
    g_push(g);
    g_clip(g, X(0), Y(990), S(1920), S(70));
    const char *tick = "MARKETS  KEEL 4,128 +1.2%   ·   WEATHER  Clear skies tonight, 18°   ·   SPORTS  City 2 – 1 Harbor at full time   ·   SCIENCE  New exoplanet found in habitable zone   ·   TECH  Software-rendered TV OS hits 60 fps   ·   ";
    float tw = text_width(FONT_SEMIBOLD, S(30), tick, 0);
    float off = fmodf(t * S(140), tw);
    for (int i = 0; i < 3; i++) g_text(g, FONT_SEMIBOLD, S(30), X(20) - off + i * tw, Y(1012), tick, WHITE, ALIGN_LEFT, 0);
    g_pop(g);
    bug(c, ch, 1);
}

/* ---------------- Sports ---------------- */
static void ch_sports(CC *c, const Channel *ch) {
    Gfx *g = c->g;
    float t = (float)c->t;
    g_rect_vgrad(g, X(0), Y(0), S(1920), S(170), 0, HEX(0x101826), HEX(0x2A3442));
    for (int i = 0; i < 160; i++) {
        float fx = hashf(i * 3) * 1920, fy = 20 + hashf(i * 5) * 140;
        g_circle(g, X(fx), Y(fy), S(4), hsv(hashf(i), 0.5f, 0.9f, 0.6f + 0.3f * sinf(t * 2 + i)));
    }
    float px0 = 0, py0 = 170, pw = 1920, ph = 910;
    for (int i = 0; i < 12; i++) g_rect(g, X(px0 + i * pw / 12), Y(py0), S(pw / 12 + 1), S(ph), 0, i & 1 ? HEX(0x2E8B3E) : HEX(0x349E46));
    uint32_t line = rgba(255, 255, 255, 200);
    g_stroke(g, X(120), Y(230), S(1680), S(790), 0, S(5), line);
    g_line(g, X(960), Y(230), X(960), Y(1020), S(5), line);
    g_stroke(g, X(960 - 130), Y(625 - 130), S(260), S(260), S(130), S(5), line);
    g_stroke(g, X(120), Y(450), S(260), S(350), 0, S(5), line);
    g_stroke(g, X(1540), Y(450), S(260), S(350), 0, S(5), line);
    float bx = 960 + sinf(t * 0.6f) * 600 + sinf(t * 1.7f) * 120, by = 625 + sinf(t * 0.9f) * 280;
    for (int team = 0; team < 2; team++)
        for (int i = 0; i < 11; i++) {
            float hx = (team ? 1800 : 120) + (team ? -1 : 1) * (80 + (i % 4) * 300 + (i == 0 ? -40 : 0));
            float hy = 625 + ((i % 5) - 2) * 160 + (i == 0 ? 0 : 40);
            float chase = (i % 3 == 0) ? 0.35f : 0.12f;
            float x = hx + (bx - hx) * chase + sinf(t * 1.3f + i + team * 3) * 30;
            float y = hy + (by - hy) * chase + cosf(t * 1.1f + i * 2) * 25;
            g_shadow(g, X(x - 14), Y(y - 6), S(28), S(28), S(14), S(8), rgba(0, 0, 0, 80));
            g_circle(g, X(x), Y(y), S(16), team ? HEX(0xFFFFFF) : HEX(0xE63946));
            g_circle(g, X(x), Y(y), S(9), team ? HEX(0x1D3557) : HEX(0xFFD166));
        }
    g_shadow(g, X(bx - 8), Y(by - 2), S(16), S(16), S(8), S(6), rgba(0, 0, 0, 120));
    g_circle(g, X(bx), Y(by - 6), S(10), WHITE);
    /* scoreboard (right of the LIVE badge) */
    g_push(g);
    g_translate(g, S(150), 0);
    ui_glass_dark(g, X(80), Y(40), S(560), S(80), S(18), 1);
    int minute = 67 + (int)(t / 10);
    char clock[16];
    snprintf(clock, sizeof clock, "%d:%02d", minute % 90, (int)fmodf(t * 6, 60));
    g_rect(g, X(100), Y(56), S(46), S(46), S(10), HEX(0xE63946));
    ktext(c, FONT_BOLD, 28, 160, 66, "CTY", WHITE, ALIGN_LEFT, 1);
    ktext(c, FONT_BLACK, 34, 290, 63, "2 – 1", WHITE, ALIGN_CENTER, 0);
    ktext(c, FONT_BOLD, 28, 370, 66, "HBR", WHITE, ALIGN_LEFT, 1);
    g_rect(g, X(450), Y(56), S(46), S(46), S(10), HEX(0x1D3557));
    ktext(c, FONT_SEMIBOLD, 28, 520, 66, clock, rgba(255, 255, 255, 220), ALIGN_LEFT, 0);
    g_pop(g);
    bug(c, ch, 1);
}

/* ---------------- Space ---------------- */
static void ch_space(CC *c, const Channel *ch) {
    Gfx *g = c->g;
    float t = (float)c->t;
    const Texture *neb = g_movies[5].blur;
    g_rect(g, X(0), Y(0), S(1920), S(1080), 0, HEX(0x05030F));
    if (neb && neb->ready) g_image(g, neb, X(0), Y(0), S(1920), S(1080), 0, 0.7f);
    for (int i = 0; i < 220; i++) {
        float z = fmodf(hashf(i * 11) - t * 0.12f * (0.5f + hashf(i * 3)), 1.f);
        if (z < 0) z += 1;
        float sx = (hashf(i * 7) - 0.5f) * 2, sy = (hashf(i * 13) - 0.5f) * 2;
        float zz = 0.05f + z;
        float px = 960 + sx / zz * 200, py = 540 + sy / zz * 200;
        float zz2 = zz + 0.04f;
        float qx = 960 + sx / zz2 * 200, qy = 540 + sy / zz2 * 200;
        int a = (int)(255 * (1 - z));
        g_line(g, X(qx), Y(qy), X(px), Y(py), S(1.5f + 2 * (1 - z)), rgba(220, 230, 255, a));
    }
    float R = 210, cx = 1400, cy = 640;
    g_glow(g, X(cx), Y(cy), S(R * 1.5f), rgba(255, 150, 90, 60));
    g_circle(g, X(cx), Y(cy), S(R), HEX(0xD9844A));
    g_push(g);
    g_clip(g, X(cx - R), Y(cy - R), S(2 * R), S(2 * R));
    for (int b = 0; b < 7; b++) {
        float yy = cy - R + b * R * 2 / 7;
        float hw = sqrtf(fmaxf(0, R * R - (yy + R / 7 - cy) * (yy + R / 7 - cy)));
        float sh = fmodf(t * 30 + b * 40, 80);
        g_rect(g, X(cx - hw + sh * 0.2f), Y(yy), S(2 * hw), S(R * 0.16f), S(R * 0.08f), b & 1 ? rgba(255, 200, 150, 70) : rgba(120, 50, 30, 60));
    }
    g_pop(g);
    g_circle(g, X(cx + 70), Y(cy + 50), S(R * 0.95f), rgba(10, 5, 20, 120));
    bug(c, ch, 0);
}

/* ---------------- Weather ---------------- */
static void ch_weather(CC *c, const Channel *ch) {
    Gfx *g = c->g;
    float t = (float)c->t;
    const Texture *map = g_weather_map;
    if (map && map->ready) {
        float pan = fmodf(t * 4, 200);
        g_image_src(g, map, 100 + pan * 0.5f, 60, 720, 405, X(0), Y(0), S(1920), S(1080), 0, 1);
    } else g_rect(g, X(0), Y(0), S(1920), S(1080), 0, HEX(0x2A6FB0));
    for (int i = 0; i < 14; i++) {
        float x = fmodf(hashf(i * 5) * 2400 + t * 40, 2400) - 240, y = 150 + hashf(i * 9) * 760;
        float r = 120 + hashf(i * 3) * 160;
        g_glow(g, X(x), Y(y), S(r), rgba(255, 255, 255, 70));
    }
    for (int i = 0; i < 70; i++) {
        float x = 300 + hashf(i * 17) * 500 + t * 40, y = fmodf(hashf(i * 23) * 400 + t * 300, 400) + 380;
        g_line(g, X(x), Y(y), X(x - 6), Y(y + 18), S(2), rgba(120, 200, 255, 160));
    }
    static const struct { float x, y; int temp; } cities[] = {{520, 380, 14}, {900, 560, 18}, {1180, 330, 11}, {700, 760, 21}, {1300, 700, 19}};
    for (int i = 0; i < 5; i++) {
        float x = cities[i].x, y = cities[i].y;
        uint32_t col = cities[i].temp > 18 ? HEX(0xFF8C42) : (cities[i].temp > 13 ? HEX(0xFFC93C) : HEX(0x4EA8DE));
        g_shadow(g, X(x - 44), Y(y - 40), S(88), S(88), S(44), S(16), rgba(0, 0, 0, 80));
        g_circle(g, X(x), Y(y), S(44), col);
        char buf[8];
        snprintf(buf, sizeof buf, "%d°", cities[i].temp);
        ktext(c, FONT_BLACK, 32, x, y - 13, buf, WHITE, ALIGN_CENTER, 0);
    }
    ui_glass(g, X(1500), Y(150), S(340), S(800), S(32), 1);
    ktext(c, FONT_BOLD, 30, 1670, 190, "5-Day Forecast", WHITE, ALIGN_CENTER, 0);
    static const char *days[] = {"Today", "Sat", "Sun", "Mon", "Tue"};
    static const int icons[] = {IC_SUN, IC_WEATHER, IC_RAIN, IC_CLOUD, IC_SUN};
    static const int hi[] = {18, 17, 13, 15, 20}, lo[] = {9, 10, 8, 7, 11};
    for (int i = 0; i < 5; i++) {
        float yy = 270 + i * 136;
        ktext(c, FONT_SEMIBOLD, 26, 1540, yy + 20, days[i], WHITE, ALIGN_LEFT, 0);
        g_icon(g, icons[i], X(1650), Y(yy + 4), S(56), icons[i] == IC_SUN ? HEX(0xFFD166) : WHITE);
        char buf[16];
        snprintf(buf, sizeof buf, "%d° / %d°", hi[i], lo[i]);
        ktext(c, FONT_MEDIUM, 24, 1810, yy + 22, buf, rgba(255, 255, 255, 220), ALIGN_RIGHT, 0);
    }
    g_rect(g, X(0), Y(960), S(1920), S(120), 0, rgba(10, 30, 70, 200));
    ktext(c, FONT_BLACK, 44, 96, 992, "WEATHER NOW", WHITE, ALIGN_LEFT, 3);
    ktext(c, FONT_MEDIUM, 30, 560, 998, "Showers clearing west to east · Mild weekend ahead", rgba(255, 255, 255, 220), ALIGN_LEFT, 0);
    bug(c, ch, 1);
}

/* ---------------- Ocean ---------------- */
static void ch_ocean(CC *c, const Channel *ch) {
    Gfx *g = c->g;
    float t = (float)c->t;
    g_rect_vgrad(g, X(0), Y(0), S(1920), S(1080), 0, HEX(0x1FA3C4), HEX(0x031B3D));
    for (int i = 0; i < 9; i++) {
        float x = 200 + i * 210 + sinf(t * 0.4f + i) * 80;
        float w = 50 + 40 * hashf(i);
        float q[] = {X(x), Y(0), X(x + w), Y(0), X(x + w * 3 + sinf(t * 0.3f + i) * 100), Y(1080), X(x + w * 0.5f + sinf(t * 0.3f + i) * 100), Y(1080)};
        g_poly(g, q, 4, rgba(200, 255, 255, 14));
    }
    for (int i = 0; i < 7; i++) {
        float bx = 140 + i * 280;
        for (int s = 0; s < 10; s++) {
            float y0 = 1080 - s * 60, y1 = y0 - 60;
            float sway0 = sinf(t * 1.1f + i + s * 0.35f) * s * 6, sway1 = sinf(t * 1.1f + i + (s + 1) * 0.35f) * (s + 1) * 6;
            g_line(g, X(bx + sway0), Y(y0), X(bx + sway1), Y(y1), S(18 - s), rgba(20, 110, 70, 220));
        }
    }
    for (int i = 0; i < 9; i++) {
        float dir = (i & 1) ? 1 : -1;
        float x = fmodf(hashf(i * 3) * 2400 + dir * t * (60 + 40 * hashf(i)), 2400);
        if (x < 0) x += 2400;
        x -= 240;
        float y = 220 + hashf(i * 7) * 600 + sinf(t + i) * 20;
        float sz = 30 + 40 * hashf(i * 5);
        uint32_t col = hsv(0.05f + hashf(i * 9) * 0.12f, 0.8f, 1.f, 1);
        float tail = sinf(t * 8 + i) * 0.3f;
        float tx = x - dir * sz * 1.1f;
        float tri[] = {X(tx), Y(y), X(tx - dir * sz * 0.7f), Y(y - sz * (0.6f + tail)), X(tx - dir * sz * 0.7f), Y(y + sz * (0.6f - tail))};
        g_poly(g, tri, 3, col);
        g_rect(g, X(x - sz * 1.2f), Y(y - sz * 0.55f), S(sz * 2.4f), S(sz * 1.1f), S(sz * 0.55f), col);
        g_circle(g, X(x + dir * sz * 0.7f), Y(y - sz * 0.12f), S(sz * 0.14f), WHITE);
        g_circle(g, X(x + dir * sz * 0.74f), Y(y - sz * 0.12f), S(sz * 0.07f), BLACK);
    }
    for (int i = 0; i < 40; i++) {
        float x = hashf(i * 5) * 1920 + sinf(t * 2 + i) * 10, y = 1080 - fmodf(hashf(i * 7) * 1080 + t * (60 + 60 * hashf(i)), 1180);
        float r = 4 + 8 * hashf(i * 3);
        g_stroke(g, X(x - r), Y(y - r), S(2 * r), S(2 * r), S(r), S(2), rgba(220, 255, 255, 150));
    }
    g_rect_vgrad(g, X(0), Y(980), S(1920), S(100), 0, rgba(10, 40, 50, 0), rgba(10, 40, 50, 255));
    bug(c, ch, 0);
}

/* ---------------- Retro / synth ---------------- */
static void ch_retro(CC *c, const Channel *ch) {
    Gfx *g = c->g;
    float t = (float)c->t;
    const Texture *bd = g_movies[3].backdrop && g_movies[3].backdrop->ready ? g_movies[3].backdrop : g_movies[3].blur;
    if (bd && bd->ready) g_image(g, bd, X(0), Y(0), S(1920), S(1080), 0, 1);
    float hy = 648;
    g_rect_vgrad(g, X(0), Y(hy), S(1920), S(1080 - hy), 0, HEX(0x1A0533), HEX(0x05010C));
    for (int i = 0; i < 14; i++) {
        float z = fmodf(i / 14.f + t * 0.25f, 1.f);
        float y = hy + (1080 - hy) * z * z;
        g_rect(g, X(0), Y(y), S(1920), S(1 + 3 * z), 0, rgba(255, 70, 200, (int)(60 + 190 * z)));
    }
    for (int i = -12; i <= 12; i++) g_line(g, X(960 + i * 20), Y(hy), X(960 + i * 260), Y(1080), S(2), rgba(255, 70, 200, 170));
    for (int i = 0; i < 24; i++) {
        float v = 0.3f + 0.7f * fabsf(sinf(t * (2 + hashf(i) * 3) + i));
        g_rect(g, X(500 + i * 38), Y(950 - v * 160), S(26), S(v * 160), S(6), rgba(255, 120, 220, 200));
    }
    ktext(c, FONT_BLACK, 40, 96, 980, "NOW PLAYING", rgba(255, 255, 255, 220), ALIGN_LEFT, 4);
    ktext(c, FONT_MEDIUM, 32, 96, 1030, "Midnight Arcade — Chrome Horizon", rgba(255, 200, 240, 230), ALIGN_LEFT, 0);
    bug(c, ch, 0);
}

/* ---------------- Kids ---------------- */
static void ch_kids(CC *c, const Channel *ch) {
    Gfx *g = c->g;
    float t = (float)c->t;
    g_rect_vgrad(g, X(0), Y(0), S(1920), S(1080), 0, HEX(0x7FD3FF), HEX(0xD9F4FF));
    float sx = 1600, sy = 200;
    for (int i = 0; i < 12; i++) {
        float a = t * 0.5f + i * PI_F / 6;
        g_line(g, X(sx + cosf(a) * 120), Y(sy + sinf(a) * 120), X(sx + cosf(a) * 180), Y(sy + sinf(a) * 180), S(14), HEX(0xFFD23F));
    }
    g_circle(g, X(sx), Y(sy), S(100), HEX(0xFFD23F));
    g_circle(g, X(sx - 34), Y(sy - 10), S(10), HEX(0x5A3A00));
    g_circle(g, X(sx + 34), Y(sy - 10), S(10), HEX(0x5A3A00));
    static const uint32_t rb[] = {0xFF5E5B, 0xFFA34D, 0xFFE15D, 0x6BD96B, 0x5DB7FF, 0x9B7BFF};
    for (int i = 0; i < 6; i++) {
        float R = 620 - i * 44;
        g_push(g);
        g_clip(g, X(0), Y(0), S(1920), S(860));
        g_stroke(g, X(700 - R), Y(860 - R), S(2 * R), S(2 * R), S(R), S(44), HEX(rb[i]));
        g_pop(g);
    }
    g_circle(g, X(300), Y(1500), S(800), HEX(0x6CCB5F));
    g_circle(g, X(1600), Y(1600), S(900), HEX(0x58B84C));
    for (int i = 0; i < 4; i++) {
        float x = 300 + i * 420 + sinf(t * 0.7f + i) * 60;
        float ph = fmodf(t * 1.6f + i * 0.4f, 1.f);
        float h = 4 * ph * (1 - ph);
        float y = 860 - h * 340;
        float squash = 1 + (ph < 0.08f || ph > 0.92f ? 0.25f : 0);
        uint32_t col = HEX(rb[(i * 2 + 1) % 6]);
        float w = 140 * squash, hh = 140 / squash;
        if (i % 3 == 0) g_circle(g, X(x), Y(y - hh / 2), S(w / 2), col);
        else if (i % 3 == 1) g_rect(g, X(x - w / 2), Y(y - hh), S(w), S(hh), S(28), col);
        else { float tri[] = {X(x), Y(y - hh * 1.1f), X(x + w * 0.6f), Y(y), X(x - w * 0.6f), Y(y)}; g_poly(g, tri, 3, col); }
        g_circle(g, X(x - 22), Y(y - hh * 0.55f), S(14), WHITE);
        g_circle(g, X(x + 22), Y(y - hh * 0.55f), S(14), WHITE);
        g_circle(g, X(x - 20), Y(y - hh * 0.55f), S(7), HEX(0x222222));
        g_circle(g, X(x + 24), Y(y - hh * 0.55f), S(7), HEX(0x222222));
    }
    bug(c, ch, 0);
}

/* ---------------- Nature / Cinema ---------------- */
static void ch_nature(CC *c, const Channel *ch) {
    Gfx *g = c->g;
    float t = (float)c->t;
    static const int scenes[] = {4, 13, 16, 8};
    int s = scenes[((int)(t / 16)) % 4];
    const Movie *m = &g_movies[s];
    ken_burns(c, m->backdrop && m->backdrop->ready ? m->backdrop : m->blur, 0.05f, (float)s);
    for (int i = 0; i < 9; i++) {
        float bx = fmodf(t * 70 + 200 + (i % 3) * 40 + (i / 3) * 30, 2300) - 200, by = 300 + (i % 3) * 30 + (i / 3) * 35 + sinf(t + i) * 8;
        float fl = sinf(t * 9 + i) * 10;
        g_line(g, X(bx - 18), Y(by - fl), X(bx), Y(by), S(3), rgba(30, 20, 30, 220));
        g_line(g, X(bx), Y(by), X(bx + 18), Y(by - fl), S(3), rgba(30, 20, 30, 220));
    }
    float in = fmodf(t, 16) < 6 ? 1 : 0;
    if (in > 0) {
        g_rect_hgrad(g, X(0), Y(900), S(1100), S(120), 0, rgba(0, 0, 0, 150), rgba(0, 0, 0, 0));
        ktext(c, FONT_BLACK, 44, 96, 924, "Mountain Kings", WHITE, ALIGN_LEFT, 0);
        ktext(c, FONT_MEDIUM, 26, 98, 980, "Episode 4 · The High Passes", rgba(255, 255, 255, 220), ALIGN_LEFT, 0);
    }
    bug(c, ch, 0);
}

static void ch_cinema(CC *c, const Channel *ch) {
    Gfx *g = c->g;
    float t = (float)c->t;
    static const int films[] = {4, 7, 2, 8, 10, 17, 12, 14};
    int idx = ((int)(t / 20)) % 8;
    const Movie *m = &g_movies[films[idx]];
    ken_burns(c, m->backdrop && m->backdrop->ready ? m->backdrop : m->blur, 0.04f, (float)idx);
    float bar = (1080 - 1920 / 2.39f) / 2;
    g_rect(g, X(0), Y(0), S(1920), S(bar), 0, BLACK);
    g_rect(g, X(0), Y(1080 - bar), S(1920), S(bar), 0, BLACK);
    float lt = fmodf(t, 20);
    if (lt < 7) {
        float a = fminf(1, lt) * fminf(1, 7 - lt);
        g_push(g);
        g_opacity(g, a);
        ktext(c, FONT_MEDIUM, 26, 96, 1080 - bar + 30, "NOW SHOWING", rgba(255, 220, 140, 255), ALIGN_LEFT, 4);
        ktext(c, FONT_BLACK, 40, 96, 1080 - bar + 70, m->title, WHITE, ALIGN_LEFT, 0);
        g_pop(g);
    }
    bug(c, ch, 0);
}

void channel_draw(Gfx *g, int ch, double t, float x, float y, float w, float h) {
    CC c = {g, x, y, w / 1920.f, t};
    const Channel *chn = &g_channels[ch];
    g_push(g);
    g_clip(g, x, y, w, h);
    switch (chn->kind) {
    case 0: ch_news(&c, chn); break;
    case 1: ch_nature(&c, chn); break;
    case 2: ch_sports(&c, chn); break;
    case 3: ch_space(&c, chn); break;
    case 4: ch_weather(&c, chn); break;
    case 5: ch_ocean(&c, chn); break;
    case 6: ch_retro(&c, chn); break;
    case 7: ch_kids(&c, chn); break;
    default: ch_cinema(&c, chn); break;
    }
    g_pop(g);
}

/* ---------------- Schedule ---------------- */
int schedule_at(int ch, int minute_of_day, Program *out) {
    static const int durs[] = {30, 60, 30, 90, 60, 30, 60, 30, 120, 30};
    int t = 0, i = 0;
    while (1) {
        int d = durs[hash32(ch * 131 + i * 7) % COUNT(durs)];
        if (t + d > minute_of_day) {
            out->start = t; out->dur = d;
            out->title = g_channels[ch].shows[hash32(ch * 17 + i * 3) % 8];
            out->index = i;
            return i;
        }
        t += d;
        i++;
    }
}
void schedule_next(int ch, const Program *p, Program *out) { schedule_at(ch, p->start + p->dur, out); }
