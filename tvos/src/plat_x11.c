/* KEEL OS — X11 backend (Xlib + MIT-SHM, double-buffered, zero-copy). */
#include "platform.h"
#ifdef KEEL_NO_X11
const Platform plat_x11 = {"x11", NULL, NULL, NULL, NULL, NULL};
#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>

static Display *dpy;
static Window win;
static GC gc;
static int W, H, use_shm;
static XImage *img[NBUF];
static XShmSegmentInfo shm[NBUF];
static Atom wm_delete;

static int x11_init(int w, int h) {
    W = w; H = h;
    /* The presenter thread and the input poller share the connection. */
    XInitThreads();
    dpy = XOpenDisplay(NULL);
    if (!dpy) return 0;
    int scr = DefaultScreen(dpy);
    Visual *vis = DefaultVisual(dpy, scr);
    int depth = DefaultDepth(dpy, scr);
    if (depth < 24) { fprintf(stderr, "x11: need a 24/32-bit visual\n"); return 0; }
    XSetWindowAttributes a;
    a.background_pixel = BlackPixel(dpy, scr);
    a.event_mask = KeyPressMask | ExposureMask | StructureNotifyMask;
    win = XCreateWindow(dpy, RootWindow(dpy, scr), 0, 0, w, h, 0, depth, InputOutput, vis, CWBackPixel | CWEventMask, &a);
    XStoreName(dpy, win, "KEEL OS");
    XSizeHints *sh = XAllocSizeHints();
    sh->flags = PMinSize | PMaxSize;
    sh->min_width = sh->max_width = w;
    sh->min_height = sh->max_height = h;
    XSetWMNormalHints(dpy, win, sh);
    XFree(sh);
    wm_delete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, win, &wm_delete, 1);
    if (getenv("KEEL_FULLSCREEN")) {
        Atom st = XInternAtom(dpy, "_NET_WM_STATE", False), fs = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
        XChangeProperty(dpy, win, st, XA_ATOM, 32, PropModeReplace, (unsigned char *)&fs, 1);
    }
    /* Hide the pointer: this is a remote-driven 10-foot UI. */
    Pixmap blank = XCreatePixmap(dpy, win, 1, 1, 1);
    XColor dummy = {0};
    Cursor cur_ = XCreatePixmapCursor(dpy, blank, blank, &dummy, &dummy, 0, 0);
    XDefineCursor(dpy, win, cur_);
    XMapWindow(dpy, win);
    gc = XCreateGC(dpy, win, 0, NULL);
    use_shm = XShmQueryExtension(dpy) && !getenv("KEEL_NO_SHM");
    for (int i = 0; i < NBUF; i++) {
        if (use_shm) {
            img[i] = XShmCreateImage(dpy, vis, depth, ZPixmap, NULL, &shm[i], w, h);
            shm[i].shmid = shmget(IPC_PRIVATE, (size_t)img[i]->bytes_per_line * h, IPC_CREAT | 0600);
            shm[i].shmaddr = img[i]->data = shmat(shm[i].shmid, NULL, 0);
            shm[i].readOnly = False;
            XShmAttach(dpy, &shm[i]);
            XSync(dpy, False);
            shmctl(shm[i].shmid, IPC_RMID, NULL);
        } else {
            char *buf = xcalloc((size_t)w * h, 4);
            img[i] = XCreateImage(dpy, vis, depth, ZPixmap, 0, buf, w, h, 32, w * 4);
        }
    }
    XSync(dpy, False);
    return 1;
}

static uint32_t *x11_buffer(int i, int *stride) {
    *stride = img[i]->bytes_per_line / 4;
    return (uint32_t *)img[i]->data;
}

static void x11_present(int i) {
    if (use_shm) XShmPutImage(dpy, win, gc, img[i], 0, 0, 0, 0, W, H, False);
    else XPutImage(dpy, win, gc, img[i], 0, 0, 0, 0, W, H);
    /* Round trip: once the server has processed the copy the buffer is free. */
    XSync(dpy, False);
}

static int x11_poll(InputEvent *ev) {
    while (XPending(dpy)) {
        XEvent e;
        XNextEvent(dpy, &e);
        if (e.type == ClientMessage && (Atom)e.xclient.data.l[0] == wm_delete) { ev->key = K_QUIT; return 1; }
        if (e.type != KeyPress) continue;
        char txt[8] = {0};
        KeySym ks;
        XLookupString(&e.xkey, txt, sizeof txt - 1, &ks, NULL);
        ev->ch = 0;
        switch (ks) {
        case XK_Up: ev->key = K_UP; return 1;
        case XK_Down: ev->key = K_DOWN; return 1;
        case XK_Left: ev->key = K_LEFT; return 1;
        case XK_Right: ev->key = K_RIGHT; return 1;
        case XK_Return: case XK_KP_Enter: ev->key = K_OK; return 1;
        case XK_Escape: ev->key = K_BACK; return 1;
        case XK_BackSpace: ev->key = K_DEL; return 1;
        case XK_Home: case XK_F1: ev->key = K_HOME; return 1;
        case XK_F2: ev->key = K_FPS; return 1;
        case XK_F5: ev->key = K_PLAY; return 1;
        case XK_Page_Up: ev->key = K_CHUP; return 1;
        case XK_Page_Down: ev->key = K_CHDN; return 1;
        case XK_F10: ev->key = K_QUIT; return 1;
        case XK_Menu: ev->key = K_MENU; return 1;
        }
        if ((e.xkey.state & ControlMask) && (ks == XK_q || ks == XK_Q)) { ev->key = K_QUIT; return 1; }
        if (txt[0] >= 32 && txt[0] < 127) { ev->key = K_CHAR; ev->ch = txt[0]; return 1; }
    }
    return 0;
}

static void x11_shutdown(void) {
    if (!dpy) return;
    if (use_shm) for (int i = 0; i < NBUF; i++) { XShmDetach(dpy, &shm[i]); shmdt(shm[i].shmaddr); }
    XCloseDisplay(dpy);
    dpy = NULL;
}

const Platform plat_x11 = {"x11", x11_init, x11_buffer, x11_present, x11_poll, x11_shutdown};
#endif
