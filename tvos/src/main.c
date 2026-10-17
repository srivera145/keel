/* KEEL OS — entry point: backend selection, frame pipeline, test driver.
 *
 *   keelos                      auto: X11 if $DISPLAY, else framebuffer
 *   keelos --x11 | --fbdev      force a backend
 *   keelos --headless --script tests/tour.keel --out shots/
 *       deterministic 60 Hz simulation driven by a script; renders every
 *       frame for real and reports frame-time statistics.
 *
 * Frame pipeline (interactive / --realtime): a presenter thread shows a
 * frame on every 60 Hz deadline, taken from a queue the renderer fills up
 * to two frames ahead into a triple-buffered pool. Each frame is simulated
 * for the exact time it will be displayed, so motion stays even, and a
 * render hiccup shorter than the queue never reaches the screen.
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
#include <pthread.h>
#include <signal.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define HZ 60.0
#define QUEUE_DEPTH 2

/* ---------------- headless backend ---------------- */
static uint32_t *hl_buf[NBUF];
static int hl_init(int w, int h) { for (int i = 0; i < NBUF; i++) hl_buf[i] = xcalloc((size_t)w * h, 4); return 1; }
static uint32_t *hl_buffer(int i, int *stride) { *stride = SCREEN_W; return hl_buf[i]; }
static void hl_present(int i) { (void)i; }
static int hl_poll(InputEvent *ev) { (void)ev; return 0; }
static void hl_shutdown(void) { for (int i = 0; i < NBUF; i++) free(hl_buf[i]); }
const Platform plat_headless = {"headless", hl_init, hl_buffer, hl_present, hl_poll, hl_shutdown};

/* ---------------- statistics ---------------- */
#define HIST 600
static float frame_hist[HIST];
static int hist_n, hist_i;
static double stat_sum, stat_max;
static long stat_frames, stat_over;
static float *all_frames;
static long all_n, all_cap;
static struct { const char *name; long n, slow; double sum, max; } per_screen[32];

static void record_frame(float ms) {
    const char *nm = app_current() ? app_current()->name : "?";
    int k = 0;
    while (k < 31 && per_screen[k].name && strcmp(per_screen[k].name, nm)) k++;
    per_screen[k].name = nm;
    per_screen[k].n++;
    per_screen[k].sum += ms;
    if (ms > per_screen[k].max) per_screen[k].max = ms;
    if (ms > 16.67f) per_screen[k].slow++;
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
    int n = MINI(hist_n, 120);
    float recent = 0;
    for (int i = 0; i < n; i++) recent += frame_hist[(hist_i - 1 - i + HIST) % HIST];
    app.frame_ms = n ? recent / n : 0;
    app.frame_p99 = percentile(frame_hist, hist_n, 0.99f);
}

/* ---------------- rendering one frame ---------------- */
static Gfx *G;
static const Platform *P;
static double t_build, t_raster;
static int slowlog, last_rendered;

static uint64_t last_hash;
static long static_frames;
static int skip_static = 1;

/* Simulate and record a frame. Returns 0 if it is identical to the frame
 * already on screen (nothing to render or present). */
static int build_frame(float dt) {
    app.time += dt;
    audio_tick(dt);
    app_update(dt);
    gfx_begin(G);
    app_draw(G);
    uint64_t h = gfx_hash(G);
    int changed = h != last_hash;
    last_hash = h;
    return changed || !skip_static;
}

static float render_frame(int bi, float dt, int prebuilt) {
    double t0 = now_sec();
    if (!prebuilt) build_frame(dt);
    double t1 = now_sec();
    int stride;
    uint32_t *buf = P->buffer(bi, &stride);
    Surface fb = {buf, SCREEN_W, SCREEN_H, stride};
    gfx_execute(G, &fb);
    double t2 = now_sec();
    app_frame_done(buf, stride);
    last_rendered = bi;
    app.cmds = gfx_cmd_count(G);
    t_build = (t1 - t0) * 1000;
    t_raster = (t2 - t1) * 1000;
    app.build_ms = (float)t_build;
    app.raster_ms = (float)t_raster;
    if (slowlog && t2 - t0 > 0.02)
        fprintf(stderr, "slow frame %.1f ms [%s]: build %.1f raster %.1f\n", (t2 - t0) * 1000, app_current() ? app_current()->name : "?", t_build, t_raster);
    return (float)((t2 - t0) * 1000);
}

/* ---------------- presenter & frame queue ---------------- */
enum { B_FREE, B_RENDER, B_QUEUED, B_SHOWN };
static struct { int state; long slot; } buf_st[NBUF];
static pthread_mutex_t qm = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t qcv = PTHREAD_COND_INITIALIZER;
static pthread_t presenter;
static int pipelined, presenter_quit, debug_present;
static double t_base;
static long last_slot = -1, static_slot = -1;
static float *intervals;
static long nint, capint, repeats, dropped;

static void sleep_until(double t) {
    double w = t - now_sec();
    if (w <= 0) return;
    struct timespec ts = {(time_t)w, (long)((w - (time_t)w) * 1e9)};
    while (nanosleep(&ts, &ts) == -1 && errno == EINTR) {}
}

static void *presenter_main(void *arg) {
    (void)arg;
    long n = 1;
    double last_t = 0;
    int fps_n = 0;
    double fps_t0 = now_sec();
    while (!presenter_quit) {
        double deadline = t_base + n / HZ;
        sleep_until(deadline);
        pthread_mutex_lock(&qm);
        /* newest queued frame meant for this slot or earlier; drop older ones */
        int pick = -1;
        for (int i = 0; i < NBUF; i++)
            if (buf_st[i].state == B_QUEUED && buf_st[i].slot <= n && (pick < 0 || buf_st[i].slot > buf_st[pick].slot)) pick = i;
        for (int i = 0; i < NBUF; i++)
            if (i != pick && buf_st[i].state == B_QUEUED && pick >= 0 && buf_st[i].slot < buf_st[pick].slot) { buf_st[i].state = B_FREE; dropped++; }
        if (pick >= 0) buf_st[pick].state = B_SHOWN;
        else if (static_slot < n) repeats++; /* a real miss, not an unchanged screen */
        pthread_mutex_unlock(&qm);
        if (pick >= 0) {
            P->present(pick);
            if (debug_present && (nint % 60) == 0) fprintf(stderr, "presenter: %ld frames shown, %ld missed\n", nint, repeats);
            double t = now_sec();
            pthread_mutex_lock(&qm);
            /* the buffer now on screen stays reserved until the next one replaces it */
            for (int i = 0; i < NBUF; i++)
                if (i != pick && buf_st[i].state == B_SHOWN) buf_st[i].state = B_FREE;
            if (last_t > 0) {
                if (nint == capint) { capint = capint ? capint * 2 : 4096; intervals = realloc(intervals, sizeof(float) * capint); }
                intervals[nint++] = (float)((t - last_t) * 1000);
            }
            last_t = t;
            pthread_cond_broadcast(&qcv);
            pthread_mutex_unlock(&qm);
            if (++fps_n >= 30) { app.fps = (float)(fps_n / (t - fps_t0)); fps_n = 0; fps_t0 = t; }
        }
        n++;
        /* if the presenter itself was starved, resynchronize to the clock */
        double behind = now_sec() - (t_base + n / HZ);
        if (behind > 0.1) n += (long)(behind * HZ);
    }
    return NULL;
}

/* Produce one frame. Pipelined: simulate it for its display slot, render it
 * into a free buffer and queue it. Otherwise: render and present directly. */
static void produce_frame(float fixed_dt) {
    InputEvent ev;
    while (P->poll(&ev)) app_input(ev.key, ev.ch);
    if (!pipelined) {
        int bi = (last_rendered + 1) % NBUF;
        record_frame(render_frame(bi, fixed_dt, 0));
        P->present(bi);
        app.fps = app.frame_ms > 16.67f ? 1000.f / app.frame_ms : 60.f;
        if (stat_frames % 30 == 0) update_overlay_stats();
        return;
    }
    pthread_mutex_lock(&qm);
    int bi = -1;
    for (;;) {
        int queued = 0;
        bi = -1;
        for (int i = 0; i < NBUF; i++) {
            if (buf_st[i].state == B_QUEUED) queued++;
            if (buf_st[i].state == B_FREE && bi < 0) bi = i;
        }
        if (bi >= 0 && queued < QUEUE_DEPTH) break;
        pthread_cond_wait(&qcv, &qm);
    }
    buf_st[bi].state = B_RENDER;
    /* the earliest slot this frame can still make, never before the last one */
    long now_slot = (long)((now_sec() - t_base) * HZ) + 1;
    long slot = last_slot < 0 ? now_slot + 1 : MAXI(last_slot + 1, now_slot);
    float dt = last_slot < 0 ? (float)(1 / HZ) : (float)((slot - last_slot) / HZ);
    last_slot = slot;
    pthread_mutex_unlock(&qm);
    double tb = now_sec();
    if (!build_frame(fminf(dt, 0.1f))) {
        /* Nothing changed: the screen already shows this frame. Keep the
         * slot's cadence without rendering or presenting. */
        pthread_mutex_lock(&qm);
        buf_st[bi].state = B_FREE;
        static_slot = slot;
        static_frames++;
        pthread_mutex_unlock(&qm);
        record_frame((float)((now_sec() - tb) * 1000));
        sleep_until(t_base + (slot - 1) / HZ);
        return;
    }
    record_frame(render_frame(bi, 0, 1));
    if (stat_frames % 30 == 0) update_overlay_stats();
    pthread_mutex_lock(&qm);
    buf_st[bi].state = B_QUEUED;
    buf_st[bi].slot = slot;
    pthread_mutex_unlock(&qm);
}

static void pipeline_start(void) {
    pipelined = 1;
    t_base = now_sec();
    pthread_create(&presenter, NULL, presenter_main, NULL);
}
static void pipeline_stop(void) {
    if (!pipelined) return;
    presenter_quit = 1;
    pthread_join(presenter, NULL);
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
static int check_failures;

static void sim_frames(int n) {
    for (int i = 0; i < n && !app.quit; i++) produce_frame(1.f / 60.f);
}

static void reset_stats(void) {
    stat_sum = stat_max = 0; stat_frames = stat_over = 0; all_n = 0;
    pthread_mutex_lock(&qm);
    nint = 0; repeats = 0; dropped = 0; static_frames = 0;
    pthread_mutex_unlock(&qm);
    memset(per_screen, 0, sizeof per_screen);
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
            char name[32];
            int count = 1;
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
            uint32_t *buf = P->buffer(last_rendered, &stride);
            png_write(fn, buf, SCREEN_W, SCREEN_H, stride);
            printf("  shot %-28s screen=%-12s frame %.2f ms (build %.2f + raster %.2f), %d cmds, %d glass\n", arg,
                   app_current() ? app_current()->name : "?", t_build + t_raster, t_build, t_raster, app.cmds, g_stat_blur_regions);
        } else if (!strcmp(cmd, "assets")) {
            double t0 = now_sec();
            while ((jobs_pending() > 0 || assets_progress() < 1.f) && now_sec() - t0 < 90) sim_frames(10);
            printf("  assets ready after %.1f s\n", now_sec() - t0);
        } else if (!strcmp(cmd, "profile")) {
            g_profile = 1;
            sim_frames(1);
            gfx_profile_dump();
            sim_frames(1);
            fprintf(stderr, "  profile (%s, single thread):\n", app_current()->name);
            gfx_profile_dump();
            g_profile = 0;
        } else if (!strcmp(cmd, "check")) {
            /* The frame must have real content: luminance spread above a floor. */
            int stride;
            uint32_t *buf = P->buffer(last_rendered, &stride);
            double s1 = 0, s2 = 0;
            long n = 0;
            for (int y = 0; y < SCREEN_H; y += 7)
                for (int x = 0; x < SCREEN_W; x += 7) {
                    uint32_t c = buf[(size_t)y * stride + x];
                    double l = 0.3 * ((c >> 16) & 255) + 0.59 * ((c >> 8) & 255) + 0.11 * (c & 255);
                    s1 += l; s2 += l * l; n++;
                }
            double sd = sqrt(s2 / n - (s1 / n) * (s1 / n));
            int ok = sd > 6;
            printf("  check %-27s %s (luminance spread %.1f)\n", app_current() ? app_current()->name : "?", ok ? "ok" : "FAIL", sd);
            if (!ok) check_failures++;
        } else if (!strcmp(cmd, "reset_stats")) reset_stats();
        else if (!strcmp(cmd, "echo")) printf("%s\n", arg);
        else if (!strcmp(cmd, "quit")) break;
        else fprintf(stderr, "%s:%d unknown command %s\n", path, lineno, cmd);
    }
    fclose(f);
    return 0;
}

static int report(void) {
    if (!stat_frames) return 0;
    float p50 = percentile(all_frames, all_n, 0.5f), p95 = percentile(all_frames, all_n, 0.95f), p99 = percentile(all_frames, all_n, 0.99f);
    printf("\nrender time: %ld frames   avg %.2f ms   p50 %.2f   p95 %.2f   p99 %.2f   max %.2f ms   over 16.7 ms: %ld (%.2f%%)\n",
           stat_frames, stat_sum / stat_frames, p50, p95, p99, stat_max, stat_over, 100.0 * stat_over / stat_frames);
    printf("per screen:\n");
    for (int k = 0; k < 32 && per_screen[k].name; k++)
        printf("  %-12s %6ld frames   avg %5.2f ms   max %6.2f ms   over 16.7 ms %ld\n", per_screen[k].name, per_screen[k].n,
               per_screen[k].sum / per_screen[k].n, per_screen[k].max, per_screen[k].slow);
    printf("render budget: %s\n", p99 <= 16.67f ? "MET (p99 render time within 16.7 ms)" : "MISSED");
    if (pipelined && nint > 10) {
        double sum = 0;
        long late = 0;
        for (long i = 0; i < nint; i++) { sum += intervals[i]; if (intervals[i] > 25.f) late++; }
        long slots = nint + repeats + static_frames;
        printf("display: %ld vsyncs   %ld new frames, %ld unchanged (skipped), %ld missed (%.2f%%), %ld dropped   p99 present interval %.2f ms\n",
               slots, nint + 1, static_frames, repeats, 100.0 * repeats / (slots ? slots : 1), dropped, percentile(intervals, nint, 0.99f));
        (void)late;
    }
    return p99 <= 16.67f;
}

/* ---------------- init (PID 1) ---------------- */
static void as_init(void) {
    mkdir("/proc", 0755); mkdir("/sys", 0755); mkdir("/dev", 0755); mkdir("/tmp", 01777);
    mount("proc", "/proc", "proc", 0, NULL);
    mount("sysfs", "/sys", "sysfs", 0, NULL);
    mount("devtmpfs", "/dev", "devtmpfs", 0, NULL);
    mount("tmpfs", "/tmp", "tmpfs", 0, NULL);
    setenv("PATH", "/bin:/sbin:/usr/bin", 0);
    setenv("KEEL_FONT_DIR", "/usr/share/fonts", 0);
    printf("KEEL OS init: pid 1, filesystems mounted\n");
}

static void on_signal(int s) { (void)s; app.quit = 1; }

int main(int argc, char **argv) {
    const char *script = NULL;
    int want = 0; /* 0 auto 1 x11 2 fbdev 3 headless */
    int threads = 0, mute = 0, realtime = 0, fps = 0, assert_budget = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--x11")) want = 1;
        else if (!strcmp(argv[i], "--fbdev")) want = 2;
        else if (!strcmp(argv[i], "--headless")) want = 3;
        else if (!strcmp(argv[i], "--script") && i + 1 < argc) script = argv[++i];
        else if (!strcmp(argv[i], "--out") && i + 1 < argc) snprintf(out_dir, sizeof out_dir, "%s", argv[++i]);
        else if (!strcmp(argv[i], "--threads") && i + 1 < argc) threads = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--realtime")) realtime = 1;
        else if (!strcmp(argv[i], "--mute")) mute = 1;
        else if (!strcmp(argv[i], "--fps")) fps = 1;
        else if (!strcmp(argv[i], "--assert-budget")) assert_budget = 1;
        else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
            printf("usage: %s [--x11|--fbdev|--headless] [--script file] [--out dir] [--threads n] [--realtime] [--mute] [--fps]\n", argv[0]);
            return 0;
        }
    }
    if (getpid() == 1) { as_init(); want = 2; }
    slowlog = getenv("KEEL_SLOWLOG") != NULL;
    debug_present = getenv("KEEL_DEBUG") != NULL;
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    if (!fonts_init()) {
        fprintf(stderr, "keelos: no usable font found (looked for Inter, DejaVu, Liberation under /usr/share/fonts; set KEEL_FONT_DIR)\n");
        return 1;
    }
    long ncpu = sysconf(_SC_NPROCESSORS_ONLN);
    if (threads <= 0) threads = clampi((int)ncpu, 1, 16);
    jobs_init(threads, MAXI(1, (int)ncpu - 1));

    if (want == 0) want = getenv("DISPLAY") ? 1 : 2;
    P = want == 1 ? &plat_x11 : (want == 2 ? &plat_fbdev : &plat_headless);
    if (!P->init || !P->init(SCREEN_W, SCREEN_H)) {
        fprintf(stderr, "keelos: %s backend unavailable", P->name);
        if (want != 3) { fprintf(stderr, ", falling back to headless\n"); P = &plat_headless; P->init(SCREEN_W, SCREEN_H); }
        else { fprintf(stderr, "\n"); return 1; }
    }
    int headless = P == &plat_headless;
    audio_init(!(mute || headless));
    if (headless && !realtime) audio_set_manual_clock(1);
    printf("KEEL OS — %s backend, %dx%d, %d render threads, font %s, audio: %s\n", P->name, SCREEN_W, SCREEN_H, threads,
           font_path(FONT_REGULAR), audio_device());

    G = gfx_new_recorder();
    app_init();
    app.set.show_fps = fps;
    assets_start();

    /* Interactive backends and --realtime use the paced pipeline; plain
     * headless runs as fast as possible with a fixed 60 Hz timestep. */
    if (!headless || realtime) pipeline_start();
    if (script) run_script(script);
    else if (headless) sim_frames(600);
    else while (!app.quit) produce_frame(1.f / 60.f);
    pipeline_stop();

    int budget_ok = report();
    audio_shutdown();
    P->shutdown();
    jobs_shutdown();
    if (getpid() == 1) { sync(); reboot(RB_POWER_OFF); for (;;) pause(); } /* PID 1 must never exit */
    if (check_failures) { printf("%d blank-frame check(s) failed\n", check_failures); return 2; }
    if (assert_budget && !budget_ok) return 3;
    return 0;
}
