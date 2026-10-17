#include "../src/gfx.h"
#include "../src/font.h"
#include "../src/jobs.h"
#include "../src/png.h"
#include "../src/raster.h"
int main(void) {
    jobs_init(4, 0);
    if (!fonts_init()) { printf("no fonts\n"); return 1; }
    for (int i = 0; i < FONT_COUNT; i++) printf("font %d: %s (%d glyphs)\n", i, font_path(i), font_glyph_count(i));
    uint32_t *px = xcalloc(SCREEN_W * SCREEN_H, 4);
    Surface fb = {px, SCREEN_W, SCREEN_H, SCREEN_W};
    Gfx *g = gfx_new_recorder();
    double best = 1e9;
    for (int it = 0; it < 30; it++) {
        double t0 = now_sec();
        gfx_begin(g);
        g_rect_grad(g, 0, 0, SCREEN_W, SCREEN_H, 0, HEX(0xff7a59), HEX(0x3a2bd6), 0, 0, SCREEN_W, SCREEN_H);
        for (int i = 0; i < 12; i++) g_glow(g, 200 + i * 140, 300 + 120 * sinf(i), 160, rgba(255, 220 - i * 10, 120 + i * 10, 140));
        for (int i = 0; i < 8; i++) { g_shadow(g, 100 + i * 220, 560, 200, 300, 18, 30, rgba(0,0,0,90)); g_rect_vgrad(g, 100 + i * 220, 550, 200, 300, 18, hsv(i / 8.f, .6f, 1, 1), hsv(i / 8.f + .2f, .8f, .5f, 1)); }
        g_backdrop(g, 160, 40, 1600, 90, 45, 10, rgba(255,255,255,50), 1.5f);
        g_stroke(g, 160, 40, 1600, 90, 45, 1.5f, rgba(255,255,255,90));
        g_text(g, FONT_SEMIBOLD, 28, 220, 75, "Home   Movies   Live TV   Music   Photos   Search   Settings", WHITE, ALIGN_LEFT, 0);
        g_text_shadow(g, FONT_BLACK, 120, 100, 200, "NORTHERN LIGHT", WHITE, ALIGN_LEFT, 4);
        g_text(g, FONT_REGULAR, 24, 100, 360, "The quick brown fox jumps over the lazy dog. Café — 4K · HDR · 2h 4m", WHITE, ALIGN_LEFT, 0);
        g_backdrop(g, 1300, 600, 500, 400, 30, 12, rgba(255,255,255,40), 1.4f);
        g_line(g, 100, 950, 1800, 1000, 6, WHITE);
        float tri[6] = {900, 900, 960, 940, 900, 980};
        g_poly(g, tri, 3, WHITE);
        gfx_execute(g, &fb);
        double t = now_sec() - t0;
        if (t < best) best = t;
    }
    printf("frame: %.2f ms (cmds %d)\n", best * 1000, gfx_cmd_count(g));
    png_write("build/gfx_test.png", px, SCREEN_W, SCREEN_H, SCREEN_W);
    return 0;
}
