/* KEEL OS — content catalog and procedurally generated artwork. */
#pragma once
#include "gfx.h"

/* Scene kinds for the procedural landscape generator. */
enum {
    SC_MOUNTAINS, SC_DESERT, SC_OCEAN, SC_CITY, SC_SPACE, SC_FOREST, SC_SYNTH, SC_AURORA,
    SC_CANYON, SC_PAPER, SC_LAKE, SC_COUNT
};
/* Time-of-day palettes. */
enum { TOD_DAWN, TOD_GOLDEN, TOD_DUSK, TOD_NIGHT, TOD_DAY, TOD_MAGIC, TOD_COUNT };
/* Hero elements placed in a scene. */
enum { EL_NONE, EL_FIGURE, EL_LIGHTHOUSE, EL_SHIP, EL_TREE, EL_TOWER, EL_BOAT, EL_PLANET };

typedef struct {
    int kind, tod, element;
    uint32_t seed;
    float sun_x, sun_y;  /* world coords: x in [0,1.78], y in [0,1] */
    int photo;           /* photographic variant: reflections, no titles */
    int reflect;
} SceneDesc;

typedef struct {
    const char *title, *tagline, *genre, *year, *rating, *runtime, *synopsis, *director;
    const char *cast[4];
    SceneDesc scene;
    int title_style;   /* 0 top, 1 bottom, 2 center */
    float score;       /* 0..5 stars */
    int is_new;
    Texture *poster;   /* 360x540 */
    Texture *backdrop; /* 1920x1080 */
    Texture *thumb;    /* 480x270 */
    Texture *blur;     /* 240x135, heavily blurred (backgrounds) */
} Movie;

typedef struct {
    const char *name, *code;
    int number, kind;
    uint32_t color;
    const char *shows[8];
    Texture *logo; /* 160x96 */
} Channel;

typedef struct {
    const char *title, *artist, *genre;
    int style, bpm, root, minor;
    uint32_t c0, c1, c2;
    const char *tracks[5];
    Texture *cover;     /* 600x600 */
    Texture *cover_blur;
} Album;

typedef struct {
    const char *title, *place, *date;
    SceneDesc scene;
    Texture *image; /* 1920x1080 */
    Texture *thumb; /* 480x270 */
    Texture *blur;  /* 240x135 blurred */
} Photo;

typedef struct {
    const char *name, *tagline;
    int icon;
    uint32_t c0, c1;
    int target; /* screen tab it opens, or -1 for splash */
    Texture *tile; /* 256x256 */
} AppInfo;

typedef struct {
    const char *name;
    int style;
    uint32_t c0, c1;
    Texture *avatar; /* 256x256 */
} Profile;

extern Movie g_movies[];
extern const int g_nmovies;
extern Channel g_channels[];
extern const int g_nchannels;
extern Album g_albums[];
extern const int g_nalbums;
extern Photo g_photos[];
extern const int g_nphotos;
extern AppInfo g_apps[];
extern const int g_napps;
extern Profile g_profiles[];
extern const int g_nprofiles;
extern const int g_featured[];
extern const int g_nfeatured;
typedef struct { int movie; float progress; const char *label; } ContinueItem;
extern const ContinueItem g_continue[];
extern const int g_ncontinue;

/* Shared generated textures. */
extern Texture *g_wallpaper;      /* bright gradient backdrop for menus */
extern Texture *g_wallpaper_blur;
extern Texture *g_grain;          /* 256x256 film grain tile */
extern Texture *g_weather_map;

/* Kick off background generation of every asset. */
void assets_start(void);
float assets_progress(void);      /* 0..1 */
int assets_essential_ready(void); /* avatars, icons, posters, featured backdrops */

/* Scene rendering (also used live by the player and Live TV). */
void scene_render(Texture *dst, const SceneDesc *d, float world_x0, float world_w, int layer_mask);
/* Layers for parallax playback: 0 sky (opaque), 1 far, 2 mid, 3 near. */
void scene_render_layers(Texture *layers[4], const SceneDesc *d, float world_w);
uint32_t scene_accent(const SceneDesc *d);   /* dominant bright color */
uint32_t scene_shadow(const SceneDesc *d);   /* deep tone */
