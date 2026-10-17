/* KEEL OS — boot animation: light particles converge into the KEEL mark,
 * the wordmark resolves, and a progress bar tracks asset generation. */
#include "ui.h"

static Texture *bg;
static float t;
static float px[90], py[90], ps[90], pd[90];

static void enter(int arg) {
    (void)arg;
    t = 0;
    if (!bg) {
        bg = tex_new(SCREEN_W, SCREEN_H);
        for (int y = 0; y < SCREEN_H; y++)
            for (int x = 0; x < SCREEN_W; x++) {
                float dx = (x - 960.f) / 1100.f, dy = (y - 500.f) / 800.f;
                float d = sqrtf(dx * dx + dy * dy);
                C3 c = c3lerp(c3hex(0x2A1A6E), c3hex(0x07051A), smoothstep(0.f, 1.f, d));
                c = c3add(c, c3scale(c3hex(0xFF7A59), 0.18f * expf(-d * d * 6.f)));
                int n = (int)(hash2i(x, y, 3) & 7) - 4;
                bg->px[(size_t)y * SCREEN_W + x] = rgba((int)(c.r * 255) + n, (int)(c.g * 255) + n, (int)(c.b * 255) + n, 255);
            }
        bg->opaque = 1;
        bg->ready = 1;
    }
    Rng r = rng_make(42);
    for (int i = 0; i < 90; i++) {
        float a = rng_f(&r) * 2 * PI_F, d = rng_range(&r, 500, 1200);
        px[i] = cosf(a) * d; py[i] = sinf(a) * d * 0.6f;
        ps[i] = rng_range(&r, 1.5f, 4.f);
        pd[i] = rng_range(&r, 0.f, 0.5f);
    }
}

static void update(float dt) {
    t += dt;
    if (t > 4.2f && assets_essential_ready()) app_reset_to(&scr_profiles, 0);
}

static void draw(Gfx *g) {
    g_image(g, bg, 0, 0, SCREEN_W, SCREEN_H, 0, 1);
    const float cx = 960, cy = 470;
    /* converging light particles with trails */
    for (int i = 0; i < 90; i++) {
        float k = ease_in_out_cubic(clampf((t - pd[i]) / 1.4f, 0, 1));
        float x = cx + px[i] * (1 - k), y = cy + py[i] * (1 - k);
        float k2 = ease_in_out_cubic(clampf((t - pd[i] - 0.08f) / 1.4f, 0, 1));
        float x2 = cx + px[i] * (1 - k2), y2 = cy + py[i] * (1 - k2);
        float a = (1 - smoothstep(0.85f, 1.f, k)) * smoothstep(0, 0.2f, t - pd[i]);
        if (a <= 0.01f) continue;
        g_line(g, x2, y2, x, y, ps[i], rgba(255, 210, 180, (int)(90 * a)));
        g_circle(g, x, y, ps[i], rgba(255, 240, 230, (int)(255 * a)));
    }
    /* bloom */
    float bloom = smoothstep(1.0f, 1.6f, t) * (0.75f + 0.25f * sinf(t * 2.2f));
    g_glow(g, cx, cy, 420, rgba(255, 120, 90, (int)(90 * bloom)));
    g_glow(g, cx, cy, 240, rgba(140, 110, 255, (int)(110 * bloom)));
    /* the mark */
    float m = ease_out_back(clampf((t - 1.1f) / 0.8f, 0, 1));
    if (m > 0.001f) {
        float s = 200 * (0.7f + 0.3f * m);
        float x = cx - s / 2, y = cy - s / 2;
        g_push(g);
        g_opacity(g, clampf((t - 1.1f) / 0.4f, 0, 1));
        g_shadow(g, x, y + 24, s, s, s * 0.3f, 60, rgba(0, 0, 0, 120));
        g_rect_grad(g, x, y, s, s, s * 0.3f, HEX(0xFF7A59), HEX(0x7B5CFF), x, y, x + s, y + s);
        g_glow(g, x + s * 0.3f, y + s * 0.25f, s * 0.5f, rgba(255, 255, 255, 80));
        float hull[] = {x + s * 0.2f, y + s * 0.58f, x + s * 0.8f, y + s * 0.58f, x + s * 0.62f, y + s * 0.78f, x + s * 0.38f, y + s * 0.78f};
        g_poly(g, hull, 4, WHITE);
        float sail[] = {x + s * 0.52f, y + s * 0.18f, x + s * 0.74f, y + s * 0.52f, x + s * 0.52f, y + s * 0.52f};
        g_poly(g, sail, 3, WHITE);
        float sail2[] = {x + s * 0.47f, y + s * 0.28f, x + s * 0.47f, y + s * 0.52f, x + s * 0.3f, y + s * 0.52f};
        g_poly(g, sail2, 3, rgba(255, 255, 255, 200));
        /* light sweep across the mark */
        float sw = clampf((t - 1.8f) / 0.9f, 0, 1);
        if (sw > 0 && sw < 1) {
            g_push(g);
            g_clip(g, x, y, s, s);
            float sx = x - s + sw * s * 2.6f;
            float band[] = {sx, y, sx + s * 0.25f, y, sx - s * 0.15f, y + s, sx - s * 0.4f, y + s};
            g_poly(g, band, 4, rgba(255, 255, 255, 90));
            g_pop(g);
        }
        g_stroke(g, x, y, s, s, s * 0.3f, 2, rgba(255, 255, 255, 120));
        g_pop(g);
    }
    /* wordmark letter by letter */
    const char *word = "KEEL";
    float fs = 92, track = 26;
    float total = text_width(FONT_BLACK, fs, word, track);
    float wx = cx - total / 2;
    for (int i = 0; i < 4; i++) {
        char ch[2] = {word[i], 0};
        float k = ease_out_cubic(clampf((t - 1.7f - i * 0.12f) / 0.6f, 0, 1));
        g_push(g);
        g_opacity(g, k);
        g_text_shadow(g, FONT_BLACK, fs, wx, cy + 160 + (1 - k) * 30, ch, WHITE, ALIGN_LEFT, 0);
        g_pop(g);
        wx += text_width(FONT_BLACK, fs, ch, 0) + track;
    }
    float ta = smoothstep(2.4f, 3.0f, t);
    g_text(g, FONT_LIGHT, 30, cx, cy + 290, "Your world, beautifully on screen.", rgba(255, 255, 255, (int)(210 * ta)), ALIGN_CENTER, 2);
    /* progress */
    float pa = smoothstep(0.4f, 1.0f, t);
    float p = assets_progress();
    float bw = 360, bx = cx - bw / 2, by = 960;
    g_rect(g, bx, by, bw, 6, 3, rgba(255, 255, 255, (int)(50 * pa)));
    g_rect_hgrad(g, bx, by, fmaxf(6, bw * fminf(1.f, p * 1.0f)), 6, 3, rgba(255, 140, 110, (int)(255 * pa)), rgba(160, 130, 255, (int)(255 * pa)));
    char msg[96];
    snprintf(msg, sizeof msg, p < 0.999f ? "Composing artwork  %d%%" : "Ready", (int)(p * 100));
    g_text(g, FONT_MEDIUM, 20, cx, by + 24, msg, rgba(255, 255, 255, (int)(150 * pa)), ALIGN_CENTER, 1);
    /* fade out */
    float out = smoothstep(3.9f, 4.2f, t) * (assets_essential_ready() ? 1.f : 0.f);
    if (out > 0) g_rect(g, 0, 0, SCREEN_W, SCREEN_H, 0, rgba(255, 255, 255, (int)(60 * out)));
}

Screen scr_boot = {"boot", -1, enter, NULL, update, draw};
