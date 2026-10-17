/* KEEL OS — render thread pool (low-latency fork/join) and background jobs. */
#define _GNU_SOURCE
#include "jobs.h"
#include "core.h"
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#define MAX_THREADS 32
static int g_spin = 4000;

static struct {
    pthread_mutex_t m;
    pthread_cond_t cv;
    pthread_cond_t done;
    ParFn fn;
    void *ctx;
    int n;
    atomic_int next;
    atomic_int active;
    atomic_uint gen;
    int nworkers;
    int quit;
    pthread_t th[MAX_THREADS];
} P;

static void run_items(int thread) {
    for (;;) {
        int i = atomic_fetch_add(&P.next, 1);
        if (i >= P.n) break;
        P.fn(P.ctx, i, thread);
    }
}

static void *worker(void *arg) {
    int id = (int)(intptr_t)arg;
    unsigned seen = 0;
    for (;;) {
        /* Spin briefly for low latency between frame phases, then sleep. */
        int spins = 0;
        while (atomic_load_explicit(&P.gen, memory_order_acquire) == seen && spins < g_spin) {
            spins++;
#if defined(__x86_64__) || defined(__i386__)
            __builtin_ia32_pause();
#endif
        }
        if (atomic_load_explicit(&P.gen, memory_order_acquire) == seen) {
            pthread_mutex_lock(&P.m);
            while (atomic_load(&P.gen) == seen && !P.quit) pthread_cond_wait(&P.cv, &P.m);
            pthread_mutex_unlock(&P.m);
        }
        if (P.quit) break;
        seen = atomic_load(&P.gen);
        run_items(id);
        if (atomic_fetch_sub(&P.active, 1) == 1) {
            pthread_mutex_lock(&P.m);
            pthread_cond_signal(&P.done);
            pthread_mutex_unlock(&P.m);
        }
    }
    return NULL;
}

void parallel_for(int n, ParFn fn, void *ctx) {
    if (n <= 0) return;
    if (P.nworkers == 0 || n == 1) {
        for (int i = 0; i < n; i++) fn(ctx, i, 0);
        return;
    }
    P.fn = fn; P.ctx = ctx; P.n = n;
    atomic_store(&P.next, 0);
    atomic_store(&P.active, P.nworkers);
    pthread_mutex_lock(&P.m);
    atomic_fetch_add_explicit(&P.gen, 1, memory_order_release);
    pthread_cond_broadcast(&P.cv);
    pthread_mutex_unlock(&P.m);
    run_items(0);
    /* Workers finish quickly; spin then block. */
    int spins = 0;
    while (atomic_load(&P.active) > 0 && spins < 20000) {
        spins++;
#if defined(__x86_64__) || defined(__i386__)
        __builtin_ia32_pause();
#endif
    }
    if (atomic_load(&P.active) > 0) {
        pthread_mutex_lock(&P.m);
        while (atomic_load(&P.active) > 0) pthread_cond_wait(&P.done, &P.m);
        pthread_mutex_unlock(&P.m);
    }
}

/* ---------------- Background jobs ---------------- */
typedef struct Job { JobFn fn; void *arg; int prio; struct Job *next; } Job;
static struct {
    pthread_mutex_t m;
    pthread_cond_t cv;
    Job *head;
    int pending;
    int quit;
    int n;
    pthread_t th[MAX_THREADS];
} B;

static void *bg_worker(void *arg) {
    (void)arg;
    /* Asset generation must never steal time from the 60 fps render path. */
    setpriority(PRIO_PROCESS, (id_t)syscall(SYS_gettid), 15);
    for (;;) {
        pthread_mutex_lock(&B.m);
        while (!B.head && !B.quit) pthread_cond_wait(&B.cv, &B.m);
        if (B.quit) { pthread_mutex_unlock(&B.m); break; }
        Job *j = B.head;
        B.head = j->next;
        pthread_mutex_unlock(&B.m);
        j->fn(j->arg);
        free(j);
        pthread_mutex_lock(&B.m);
        B.pending--;
        pthread_mutex_unlock(&B.m);
    }
    return NULL;
}

void job_submit(JobFn fn, void *arg, int prio) {
    if (B.n == 0) { fn(arg); return; }
    Job *j = xmalloc(sizeof *j);
    j->fn = fn; j->arg = arg; j->prio = prio; j->next = NULL;
    pthread_mutex_lock(&B.m);
    /* Higher priority first; FIFO within a priority. */
    Job **pp = &B.head;
    while (*pp && (*pp)->prio >= prio) pp = &(*pp)->next;
    j->next = *pp;
    *pp = j;
    B.pending++;
    pthread_cond_signal(&B.cv);
    pthread_mutex_unlock(&B.m);
}

int jobs_pending(void) {
    pthread_mutex_lock(&B.m);
    int n = B.pending;
    pthread_mutex_unlock(&B.m);
    return n;
}

int jobs_render_threads(void) { return P.nworkers + 1; }

void jobs_init(int render_threads, int bg_threads) {
    if (getenv("KEEL_SPIN")) g_spin = atoi(getenv("KEEL_SPIN"));
    pthread_mutex_init(&P.m, NULL);
    pthread_cond_init(&P.cv, NULL);
    pthread_cond_init(&P.done, NULL);
    P.nworkers = clampi(render_threads - 1, 0, MAX_THREADS);
    for (int i = 0; i < P.nworkers; i++) pthread_create(&P.th[i], NULL, worker, (void *)(intptr_t)(i + 1));
    pthread_mutex_init(&B.m, NULL);
    pthread_cond_init(&B.cv, NULL);
    B.n = clampi(bg_threads, 0, MAX_THREADS);
    for (int i = 0; i < B.n; i++) pthread_create(&B.th[i], NULL, bg_worker, NULL);
}

void jobs_shutdown(void) {
    pthread_mutex_lock(&P.m);
    P.quit = 1;
    atomic_fetch_add(&P.gen, 1);
    pthread_cond_broadcast(&P.cv);
    pthread_mutex_unlock(&P.m);
    for (int i = 0; i < P.nworkers; i++) pthread_join(P.th[i], NULL);
    pthread_mutex_lock(&B.m);
    B.quit = 1;
    pthread_cond_broadcast(&B.cv);
    pthread_mutex_unlock(&B.m);
    for (int i = 0; i < B.n; i++) pthread_join(B.th[i], NULL);
}

double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}
void *xmalloc(size_t n) {
    void *p = malloc(n ? n : 1);
    if (!p) { fprintf(stderr, "out of memory\n"); abort(); }
    return p;
}
void *xcalloc(size_t n, size_t s) {
    void *p = calloc(n ? n : 1, s ? s : 1);
    if (!p) { fprintf(stderr, "out of memory\n"); abort(); }
    return p;
}
