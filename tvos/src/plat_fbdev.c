/* KEEL OS — bare-metal backend: Linux framebuffer (/dev/fb0) + evdev input.
 * Lets KEEL run as the only UI on a device, straight from init, with no
 * display server. Supports 32/16 bpp, page-flip panning when the driver
 * provides a double-height virtual framebuffer, and FBIO_WAITFORVSYNC. */
#define _GNU_SOURCE
#include "platform.h"
#include <dirent.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <linux/kd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

static int fb = -1, tty = -1;
static uint8_t *fbmem;
static size_t fbsize;
static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;
static uint32_t *back;
static int W, H, flip, page;
static int inputs[16], ninputs;
static int shift;

static int fb_init(int w, int h) {
    W = w; H = h;
    const char *dev = getenv("KEEL_FBDEV") ? getenv("KEEL_FBDEV") : "/dev/fb0";
    fb = open(dev, O_RDWR);
    if (fb < 0) return 0;
    if (ioctl(fb, FBIOGET_VSCREENINFO, &vinfo) || ioctl(fb, FBIOGET_FSCREENINFO, &finfo)) { close(fb); return 0; }
    if (vinfo.bits_per_pixel != 32 && vinfo.bits_per_pixel != 16) { fprintf(stderr, "fbdev: unsupported %u bpp\n", vinfo.bits_per_pixel); close(fb); return 0; }
    /* Try for a double-height virtual screen to flip pages without tearing. */
    struct fb_var_screeninfo v2 = vinfo;
    v2.yres_virtual = vinfo.yres * 2;
    if (ioctl(fb, FBIOPUT_VSCREENINFO, &v2) == 0) { ioctl(fb, FBIOGET_VSCREENINFO, &vinfo); ioctl(fb, FBIOGET_FSCREENINFO, &finfo); }
    flip = vinfo.yres_virtual >= vinfo.yres * 2;
    fbsize = (size_t)finfo.line_length * vinfo.yres_virtual;
    fbmem = mmap(NULL, fbsize, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);
    if (fbmem == MAP_FAILED) { close(fb); return 0; }
    back = xcalloc((size_t)w * h, 4);
    tty = open("/dev/tty0", O_RDWR);
    if (tty >= 0) ioctl(tty, KDSETMODE, KD_GRAPHICS);
    DIR *d = opendir("/dev/input");
    if (d) {
        struct dirent *e;
        while ((e = readdir(d)) && ninputs < 16) {
            if (strncmp(e->d_name, "event", 5)) continue;
            char p[300];
            snprintf(p, sizeof p, "/dev/input/%s", e->d_name);
            int fd = open(p, O_RDONLY | O_NONBLOCK);
            if (fd >= 0) inputs[ninputs++] = fd;
        }
        closedir(d);
    }
    fprintf(stderr, "fbdev: %ux%u %ubpp, %s, %d input devices\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel,
            flip ? "page flipping" : "single buffer", ninputs);
    return 1;
}

static uint32_t *fb_acquire(int *stride) { *stride = W; return back; }

static void fb_present(void) {
    int target = flip ? page ^ 1 : 0;
    uint8_t *base = fbmem + (size_t)target * vinfo.yres * finfo.line_length;
    int ox = ((int)vinfo.xres - W) / 2, oy = ((int)vinfo.yres - H) / 2;
    int sx = ox < 0 ? -ox : 0, sy = oy < 0 ? -oy : 0;
    int cw = MINI(W - sx, (int)vinfo.xres), ch = MINI(H - sy, (int)vinfo.yres);
    for (int y = 0; y < ch; y++) {
        const uint32_t *src = back + (size_t)(y + sy) * W + sx;
        uint8_t *dst = base + (size_t)(y + MAXI(oy, 0)) * finfo.line_length;
        if (vinfo.bits_per_pixel == 32) {
            uint32_t *d32 = (uint32_t *)dst + MAXI(ox, 0);
            if (vinfo.red.offset == 16) memcpy(d32, src, (size_t)cw * 4);
            else for (int x = 0; x < cw; x++) { uint32_t c = src[x]; d32[x] = ((c >> 16) & 255) | (c & 0xFF00) | ((c & 255) << 16); }
        } else {
            uint16_t *d16 = (uint16_t *)dst + MAXI(ox, 0);
            for (int x = 0; x < cw; x++) { uint32_t c = src[x]; d16[x] = (uint16_t)(((c >> 8) & 0xF800) | ((c >> 5) & 0x7E0) | ((c >> 3) & 0x1F)); }
        }
    }
    if (flip) {
        vinfo.yoffset = (uint32_t)target * vinfo.yres;
        ioctl(fb, FBIOPAN_DISPLAY, &vinfo);
        page = target;
    }
    int arg = 0;
    ioctl(fb, FBIO_WAITFORVSYNC, &arg);
}

static int map_code(int code, int *ch) {
    static const char row1[] = "qwertyuiop", row2[] = "asdfghjkl", row3[] = "zxcvbnm";
    *ch = 0;
    switch (code) {
    case KEY_UP: return 1;
    case KEY_DOWN: return 2;
    case KEY_LEFT: return 3;
    case KEY_RIGHT: return 4;
    case KEY_ENTER: case KEY_KPENTER: case KEY_OK: case KEY_SELECT: return 5;
    case KEY_ESC: case KEY_BACK: case KEY_EXIT: return 6;
    case KEY_HOMEPAGE: case KEY_F1: return 7;
    case KEY_PLAYPAUSE: case KEY_PLAY: case KEY_PAUSE: case KEY_F5: return 8;
    case KEY_MENU: case KEY_COMPOSE: return 9;
    case KEY_BACKSPACE: return 11;
    case KEY_F2: return 12;
    case KEY_F10: return 13;
    case KEY_CHANNELUP: case KEY_PAGEUP: return 14;
    case KEY_CHANNELDOWN: case KEY_PAGEDOWN: return 15;
    case KEY_SPACE: *ch = ' '; return 10;
    }
    if (code >= KEY_1 && code <= KEY_9) { *ch = '1' + code - KEY_1; return 10; }
    if (code == KEY_0) { *ch = '0'; return 10; }
    if (code >= KEY_Q && code <= KEY_P) { *ch = row1[code - KEY_Q]; return 10; }
    if (code >= KEY_A && code <= KEY_L) { *ch = row2[code - KEY_A]; return 10; }
    if (code >= KEY_Z && code <= KEY_M) { *ch = row3[code - KEY_Z]; return 10; }
    return 0;
}

static int fb_poll(InputEvent *ev) {
    for (int i = 0; i < ninputs; i++) {
        struct input_event ie;
        while (read(inputs[i], &ie, sizeof ie) == (ssize_t)sizeof ie) {
            if (ie.type != EV_KEY) continue;
            if (ie.code == KEY_LEFTSHIFT || ie.code == KEY_RIGHTSHIFT) { shift = ie.value != 0; continue; }
            if (ie.value == 0) continue; /* release */
            int ch, k = map_code(ie.code, &ch);
            if (!k) continue;
            static const int table[] = {K_NONE, K_UP, K_DOWN, K_LEFT, K_RIGHT, K_OK, K_BACK, K_HOME, K_PLAY,
                                        K_MENU, K_CHAR, K_DEL, K_FPS, K_QUIT, K_CHUP, K_CHDN};
            ev->key = table[k];
            ev->ch = (shift && ch >= 'a' && ch <= 'z') ? ch - 32 : ch;
            return 1;
        }
    }
    return 0;
}

static void fb_shutdown(void) {
    if (tty >= 0) { ioctl(tty, KDSETMODE, KD_TEXT); close(tty); }
    for (int i = 0; i < ninputs; i++) close(inputs[i]);
    if (fbmem) munmap(fbmem, fbsize);
    if (fb >= 0) close(fb);
    free(back);
}

const Platform plat_fbdev = {"fbdev", fb_init, fb_acquire, fb_present, fb_poll, fb_shutdown};
