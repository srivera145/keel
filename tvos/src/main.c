/* KEEL OS — entry point: backend selection, frame loop, headless test driver.
 *
 *   keelos                      auto: X11 if $DISPLAY, else framebuffer
 *   keelos --x11 | --fbdev      force a backend
 *   keelos --headless --script tests/tour.keel --out shots/
 *       deterministic 60 Hz simulation driven by a script; renders every
 *       frame for real and reports frame-time statistics.
 */
#define _GNU_SOURCE
#include "app.h"
#include "assets.h"
#include "audio.h"
#include "font.h"
#include "jobs.h"
#include "png.h"
#include "ui.h"
#include <errno.h>
#include <signal.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/* ---------------- headless backend ---------------- */
static uint32_t *hl_buf;
static int hl_init(int w, int h) { hl_buf = xcalloc((size_t)w * h, 4); return 1; }
static uint32_t *hl_acquire(int *stride) { *stride = SCREEN_W; return hl_buf; }
static void hl_present(void) {}
static int hl_poll(InputEvent *ev) { (void)ev; return 0; }
static void hl_shutdown(void) { free(hl_buf); }
const Platform plat_headless = {"headless", hl_init, hl_acquire, hl_present, hl_poll, hl_shutdown};

/* ---------------- frame statistics ---------------- */
#define HIST 600
static float frame_hist[HIST];
static int hist_n, hist_i;
static double stat_sum, stat_max;
static long stat_frames, stat_over;
static float *all_frames;
static long all_n, all_cap;

static void record_frame(float ms) {
    frame_hist[hist_i] = ms;
    hist_i = (hist_i + 1) % HIST;
    if (hist_n < HIST) hist_n++;
    stat_sum += ms;
    stat_frames++;
    if (ms > stat_max) stat_max = ms;
    if (ms > 16.67f) stat_over++;
    if (all_n == all_cap) { all_cap = all_cap ? all_cap * 2 : 4096; all_frames = realloc(all_frames, sizeof(float) * all_cap); }
    all_frames[all_n++] = ms;
}
static int cmpf(const void *a, const void *b) { float x = *(const float *)a, y = *(const float *)b; return x < y ? -1 : x > y; }
static float percentile(float *v, long n, float p) {
    if (!n) return 0;
    float *c = xmalloc(sizeof(float) * n);
    memcpy(c, v, sizeof(float) * n);
    qsort(c, n, sizeof(float), cmpf);
    float r = c[(long)fminf((float)(n - 1), p * n)];
    free(c);
    return r;
}
static void update_overlay_stats(void) {
    float sum = 0;
    for (int i = 0; i < hist_n; i++) sum += frame_hist[i];
    int n = MINI(hist_n, 120);
    float recent = 0;
    for (int i = 0; i < n; i++) recent += frame_hist[(hist_i - 1 - i + HIST) % HIST];
    app.frame_ms = n ? recent / n : 0;
    app.frame_p99 = percentile(frame_hist, hist_n, 0.99f);
}

/* ---------------- one frame ---------------- */
static Gfx *G;
static const Platform *P;
static double t_build, t_raster;

static float run_frame(float dt) {
    double t0 = now_sec();
    app.time += dt;
    audio_tick(dt);
    app_update(dt);
    gfx_begin(G);
    app_draw(G);
    double t1 = now_sec();
    int stride;
    uint32_t *buf = P->acquire(&stride);
    Surface fb = {buf, SCREEN_W, SCREEN_H, stride};
    gfx_execute(G, &fb);
    double t2 = now_sec();
    P->present();
    app_frame_done(buf, stride);
    app.cmds = gfx_cmd_count(G);
    t_build = (t1 - t0) * 1000;
    t_raster = (t2 - t1) * 1000;
    app.build_ms = (float)t_build;
    app.raster_ms = (float)t_raster;
    return (float)((t2 - t0) * 1000);
}

/* ---------------- headless script ---------------- */
static int key_from_name(const char *s) {
    static const struct { const char *n; int k; } T[] = {
        {"up", K_UP}, {"down", K_DOWN}, {"left", K_LEFT}, {"right", K_RIGHT}, {"ok", K_OK}, {"enter", K_OK},
        {"back", K_BACK}, {"home", K_HOME}, {"play", K_PLAY}, {"del", K_DEL}, {"fps", K_FPS}, {"menu", K_MENU},
        {"chup", K_CHUP}, {"chdn", K_CHDN},
    };
    for (int i = 0; i < COUNT(T); i++) if (!strcmp(T[i].n, s)) return T[i].k;
    return K_NONE;
}

static char out_dir[512] = "build/shots";
static int realtime;

static void sim_frames(int n) {
    for (int i = 0; i < n && !app.quit; i++) {
        double t0 = now_sec();
        float ms = run_frame(1.f / 60.f);
        record_frame(ms);
        if (realtime) {
            double left = 1.0 / 60 - (now_sec() - t0);
            if (left > 0) usleep((useconds_t)(left * 1e6));
        }
    }
}

static int run_script(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { fprintf(stderr, "cannot open script %s\n", path); return 1; }
    char line[512];
    int lineno = 0;
    mkdir(out_dir, 0755);
    while (fgets(line, sizeof line, f) && !app.quit) {
        lineno++;
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        p[strcspn(p, "\r\n")] = 0;
        if (!*p || *p == '#') continue;
        char cmd[64] = {0}, arg[400] = {0};
        sscanf(p, "%63s %399[^\n]", cmd, arg);
        if (!strcmp(cmd, "wait")) sim_frames((int)(atof(arg) * 60 + 0.5));
        else if (!strcmp(cmd, "key")) {
            char name[32]; int count = 1;
            if (sscanf(arg, "%31s %d", name, &count) < 1) continue;
            int k = key_from_name(name);
            if (k == K_NONE) { fprintf(stderr, "%s:%d unknown key %s\n", path, lineno, name); continue; }
            for (int i = 0; i < count; i++) { app_input(k, 0); sim_frames(9); }
        } else if (!strcmp(cmd, "type")) {
            for (char *c = arg; *c; c++) { app_input(K_CHAR, *c); sim_frames(4); }
        } else if (!strcmp(cmd, "shot")) {
            char fn[1024];
            snprintf(fn, sizeof fn, "%s/%s", out_dir, arg);
            int stride;
            uint32_t *buf = P->acquire(&stride);
            png_write(fn, buf, SCREEN_W, SCREEN_H, stride);
            printf("  shot %-28s screen=%-12s frame %.2f ms (build %.2f + raster %.2f), %d cmds, %d glass\n", arg,
                   app_current() ? app_current()->name : "?", t_build + t_raster, t_build, t_raster, app.cmds, g_stat_blur_regions);
        } else if (!strcmp(cmd, "profile")) {
            g_profile = 1;
            sim_frames(1);
            gfx_profile_dump();
            sim_frames(1);
            fprintf(stderr, "  profile (%s, single thread):\n", app_current()->name);
            gfx_profile_dump();
            g_profile = 0;
        } else if (!strcmp(cmd, "assets")) {
            double t0 = now_sec();
            while ((jobs_pending() > 0 || assets_progress() < 1.f) && now_sec() - t0 < 60) sim_frames(10);
            printf("  assets ready after %.1f s\n", now_sec() - t0);
        } else if (!strcmp(cmd, "reset_stats")) {
            stat_sum = stat_max = 0; stat_frames = stat_over = 0; all_n = 0;
        } else if (!strcmp(cmd, "echo")) printf("%s\n", arg);
        else if (!strcmp(cmd, "quit")) break;
        else fprintf(stderr, "%s:%d unknown command %s\n", path, lineno, cmd);
    }
    fclose(f);
    return 0;
}

/* ---------------- init (PID 1) ---------------- */
static void as_init(void) {
    mount("proc", "/proc", "proc", 0, NULL);
    mount("sysfs", "/sys", "sysfs", 0, NULL);
    mount("devtmpfs", "/dev", "devtmpfs", 0, NULL);
    setenv("PATH", "/bin:/sbin:/usr/bin", 0);
}

static void on_signal(int s) { (void)s; app.quit = 1; }

int main(int argc, char **argv) {
    const char *script = NULL;
    int want = 0; /* 0 auto 1 x11 2 fbdev 3 headless */
    int threads = 0, mute = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--x11")) want = 1;
        else if (!strcmp(argv[i], "--fbdev")) want = 2;
        else if (!strcmp(argv[i], "--headless")) want = 3;
        else if (!strcmp(argv[i], "--script") && i + 1 < argc) script = argv[++i];
        else if (!strcmp(argv[i], "--out") && i + 1 < argc) snprintf(out_dir, sizeof out_dir, "%s", argv[++i]);
        else if (!strcmp(argv[i], "--threads") && i + 1 < argc) threads = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--realtime")) realtime = 1;
        else if (!strcmp(argv[i], "--mute")) mute = 1;
        else if (!strcmp(argv[i], "--fps")) app.set.show_fps = 1;
        else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
            printf("usage: %s [--x11|--fbdev|--headless] [--script file] [--out dir] [--threads n] [--realtime] [--mute]\n", argv[0]);
            return 0;
        }
    }
    if (getpid() == 1) { as_init(); want = 2; }
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    if (!fonts_init()) {
        fprintf(stderr, "keelos: no usable font found (looked for Inter, DejaVu, Liberation under /usr/share/fonts; set KEEL_FONT_DIR)\n");
        return 1;
    }
    long ncpu = sysconf(_SC_NPROCESSORS_ONLN);
    if (threads <= 0) threads = (int)clampi((int)ncpu, 1, 16);
    jobs_init(threads, MAXI(1, (int)ncpu - 1));

    if (want == 0) want = getenv("DISPLAY") ? 1 : 2;
    P = want == 1 ? &plat_x11 : (want == 2 ? &plat_fbdev : &plat_headless);
    if (!P->init || !P->init(SCREEN_W, SCREEN_H)) {
        fprintf(stderr, "keelos: %s backend unavailable", P->name);
        if (want != 3) { fprintf(stderr, ", falling back to headless\n"); P = &plat_headless; P->init(SCREEN_W, SCREEN_H); }
        else { fprintf(stderr, "\n"); return 1; }
    }
    audio_init(!(mute || P == &plat_headless));
    if (P == &plat_headless) audio_set_manual_clock(1);
    printf("KEEL OS — %s backend, %dx%d, %d render threads, font %s, audio: %s\n", P->name, SCREEN_W, SCREEN_H, threads,
           font_path(FONT_REGULAR), audio_device());

    G = gfx_new_recorder();
    {
        int set_fps = app.set.show_fps;
        app_init();
        app.set.show_fps = set_fps;
    }
    assets_start();

    if (P == &plat_headless) {
        if (script) run_script(script);
        else sim_frames(600);
    } else {
        double next = now_sec(), last = next;
        while (!app.quit) {
            InputEvent ev;
            while (P->poll(&ev)) app_input(ev.key, ev.ch);
            double t = now_sec();
            float dt = (float)clampf((float)(t - last), 0.f, 0.1f);
            last = t;
            float ms = run_frame(dt);
            record_frame(ms);
            if (stat_frames % 30 == 0) update_overlay_stats();
            /* Pace to 60 Hz (vsync where the backend provides it). */
            next += 1.0 / 60.0;
            double wait = next - now_sec();
            if (wait > 0) {
                struct timespec ts = {(time_t)wait, (long)((wait - (time_t)wait) * 1e9)};
                while (nanosleep(&ts, &ts) == -1 && errno == EINTR) {}
            } else if (wait < -0.05) next = now_sec();
            /* Displayed fps from real present intervals. */
            static double fps_t0;
            static int fps_n;
            fps_n++;
            if (t - fps_t0 >= 0.5) { app.fps = (float)(fps_n / (t - fps_t0)); fps_n = 0; fps_t0 = t; }
        }
    }

    if (stat_frames) {
        float p50 = percentile(all_frames, all_n, 0.5f), p95 = percentile(all_frames, all_n, 0.95f), p99 = percentile(all_frames, all_n, 0.99f);
        printf("\nframes: %ld   avg %.2f ms   p50 %.2f   p95 %.2f   p99 %.2f   max %.2f ms   over 16.7ms: %ld (%.2f%%)\n",
               stat_frames, stat_sum / stat_frames, p50, p95, p99, stat_max, stat_over, 100.0 * stat_over / stat_frames);
        printf("60 fps budget: %s\n", p99 <= 16.67f ? "MET (p99 within 16.7 ms)" : "MISSED");
    }
    audio_shutdown();
    P->shutdown();
    jobs_shutdown();
    return 0;
}
