/* KEEL OS — application shell: screens, navigation stack, settings. */
#pragma once
#include "gfx.h"
#include "platform.h"

enum { TAB_HOME, TAB_MOVIES, TAB_LIVE, TAB_MUSIC, TAB_PHOTOS, TAB_SEARCH, TAB_SETTINGS, TAB_COUNT };

typedef struct Screen {
    const char *name;
    int tab;                         /* top-level tab index, or -1 */
    void (*enter)(int arg);
    int (*input)(int key, int ch);   /* return 1 if handled */
    void (*update)(float dt);
    void (*draw)(Gfx *g);
} Screen;

typedef struct {
    int accent;          /* index into accent palette */
    int reduce_motion;
    int show_fps;
    int glass;           /* 0..4 blur strength */
    int clock24;
    int ui_sounds;
    int volume;          /* 0..10 */
    int captions;
    int autoplay;
    int screensaver;     /* minutes, 0 = off */
} Settings;

typedef struct {
    double time;         /* seconds since boot (simulated in headless) */
    float dt;
    int profile;
    int nav_focused;
    int nav_index;
    Settings set;
    int quit;
    /* perf */
    float fps, frame_ms, frame_p99, build_ms, raster_ms;
    int cmds;
    double idle_since;
    float nav_scrim;     /* 0..1: frosted band behind the nav when content scrolls under it */
} App;

extern App app;

extern Screen scr_boot, scr_profiles, scr_home, scr_movies, scr_details, scr_player, scr_live, scr_live_full,
    scr_music, scr_nowplaying, scr_photos, scr_viewer, scr_search, scr_settings, scr_appsplash;

void app_init(void);
void app_push(Screen *s, int arg);
void app_back(void);
void app_tab(int tab);           /* switch top-level tab */
void app_reset_to(Screen *s, int arg);
Screen *app_current(void);
void app_input(int key, int ch);
void app_update(float dt);
void app_draw(Gfx *g);
/* Called by main after each frame so transitions can snapshot it. */
void app_frame_done(const uint32_t *px, int stride);
void app_toast(const char *msg, int icon);
int app_wall_minutes(void);     /* minutes since midnight */
void app_clock_str(char *buf, int n);

uint32_t accent(void);
uint32_t accent_n(int i);
const char *accent_name(int i);
#define ACCENT_COUNT 5
float motion(void);              /* 1, or small when reduce motion is on */
