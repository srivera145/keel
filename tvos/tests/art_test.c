#include "../src/assets.h"
#include "../src/font.h"
#include "../src/icons.h"
#include "../src/jobs.h"
#include "../src/png.h"
int main(int argc, char **argv) {
    jobs_init(1, 0);
    fonts_init();
    int mode = argc > 1 ? atoi(argv[1]) : 0;
    Texture *sheet = tex_new(1920, 1080);
    tex_clear(sheet, 0xFF202020);
    Gfx *g = gfx_new_immediate(tex_surface(sheet));
    double t0 = now_sec();
    if (mode == 0) { // backdrops: 12 at 480x270 (render full res then downscale)
        for (int i = 0; i < 12; i++) {
            Movie *m = &g_movies[i * 2 % g_nmovies + (i >= 12 ? 1 : 0)];
            if (argc > 2) m = &g_movies[(i + atoi(argv[2])) % g_nmovies];
            Texture *t = tex_new(1920, 1080);
            double a = now_sec();
            scene_render(t, &m->scene, 0, 1.7778f, 15);
            printf("%-24s %.0f ms\n", m->title, (now_sec() - a) * 1000);
            Texture *s = tex_downscale(t, 480, 270);
            g_image(g, s, (i % 4) * 480, (i / 4) * 270, 480, 270, 0, 1);
            tex_free(t); tex_free(s);
        }
    } else if (mode == 1) { // posters
        for (int i = 0; i < 24 && i < 16; i++) {
            Movie *m = &g_movies[(i + (argc > 2 ? atoi(argv[2]) : 0)) % g_nmovies];
            extern Texture *render_poster_pub(Movie *);
            Texture *p = render_poster_pub(m);
            g_image(g, p, (i % 8) * 240, (i / 8) * 360, 240, 360, 0, 1);
        }
    } else if (mode == 2) { // albums, avatars, apps, photos
        extern Texture *render_cover_pub(Album *, int);
        for (int i = 0; i < 8; i++) { Texture *c = render_cover_pub(&g_albums[i], i); g_image(g, c, i * 240, 0, 240, 240, 0, 1); }
        for (int i = 0; i < 4; i++) { Texture *c = gen_avatar(i); g_image(g, c, i * 200, 260, 180, 180, 90, 1); }
        for (int i = 0; i < 12; i++) { Texture *c = gen_app_tile(i); g_image(g, c, 820 + (i % 6) * 180, 260 + (i / 6) * 180, 160, 160, 34, 1); }
        for (int i = 0; i < IC_COUNT; i++) { g_icon(g, i, 20 + (i % 27) * 70, 640 + (i / 27) * 80, 56, WHITE); }
        for (int i = 0; i < 6; i++) {
            Photo *p = &g_photos[i + (argc > 2 ? atoi(argv[2]) : 0)]; p->scene.photo = 1;
            Texture *t = tex_new(1920, 1080); scene_render(t, &p->scene, 0, 1.7778f, 15);
            Texture *s = tex_downscale(t, 320, 180); g_image(g, s, i * 320, 860, 320, 180, 0, 1); tex_free(t);
        }
    }
    printf("total %.0f ms\n", (now_sec() - t0) * 1000);
    png_write(argc > 3 ? argv[3] : "build/art.png", sheet->px, 1920, 1080, 1920);
    return 0;
}
