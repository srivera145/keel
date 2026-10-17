/* KEEL OS — vector icon set, app tiles and avatars, all drawn in code. */
#pragma once
#include "gfx.h"

enum {
    IC_PLAY, IC_PAUSE, IC_PREV, IC_NEXT, IC_REW, IC_FFW, IC_PLUS, IC_CHECK, IC_INFO, IC_SEARCH,
    IC_GEAR, IC_HOME, IC_FILM, IC_TV, IC_MUSIC, IC_PHOTO, IC_BACK, IC_CHEV_L, IC_CHEV_R, IC_CHEV_D,
    IC_CHEV_U, IC_STAR, IC_HEART, IC_WIFI, IC_VOLUME, IC_SHUFFLE, IC_REPEAT, IC_USER, IC_GRID, IC_WEATHER,
    IC_GAMEPAD, IC_FITNESS, IC_NEWS, IC_MIC, IC_KIDS, IC_DISPLAY, IC_ACCESS, IC_BELL, IC_POWER, IC_SLIDESHOW,
    IC_DELETE, IC_SPACE, IC_GLOBE, IC_CC, IC_SUN, IC_CLOUD, IC_RAIN, IC_SNOW, IC_BOLT, IC_PALETTE,
    IC_LIST, IC_SPARKLE, IC_LIVE, IC_COUNT
};

const Mask *icon_mask(int id, int size);
/* Draw icon in a size x size box at (x,y). */
void g_icon(Gfx *g, int id, float x, float y, float size, uint32_t c);
/* Composite an icon straight into a surface (offline art). */
void icon_blit(Surface *s, int id, float x, float y, float size, uint32_t c);

struct AppInfo;
struct Profile;
Texture *gen_app_tile(int app_index);
Texture *gen_avatar(int profile_index);
