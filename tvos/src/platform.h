/* KEEL OS — display/input backends. */
#pragma once
#include "core.h"

enum {
    K_NONE, K_UP, K_DOWN, K_LEFT, K_RIGHT, K_OK, K_BACK, K_HOME,
    K_PLAY, K_MENU, K_CHAR, K_DEL, K_FPS, K_QUIT, K_CHUP, K_CHDN
};

typedef struct { int key; int ch; } InputEvent;

typedef struct Platform {
    const char *name;
    int (*init)(int w, int h);
    /* Buffer to render the next frame into (may be a shared-memory surface). */
    uint32_t *(*acquire)(int *stride);
    void (*present)(void);
    int (*poll)(InputEvent *ev);
    void (*shutdown)(void);
} Platform;

extern const Platform plat_x11;
extern const Platform plat_fbdev;
extern const Platform plat_headless;
