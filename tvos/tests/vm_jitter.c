/* Measures scheduling stalls of the host itself: N threads spin on the clock
 * and report the longest gap they observed. On real hardware gaps stay well
 * under 1 ms; in an oversubscribed VM they reveal host preemption that no
 * program running inside it can avoid. Usage: vm_jitter [threads] [seconds] */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static double secs;
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec * 1e-9; }
static void *run(void *a) {
    int id = (int)(long)a;
    double end = now() + secs, last = now(), mx = 0;
    long over17 = 0;
    for (;;) {
        double t = now(), g = t - last;
        if (g > mx) mx = g;
        if (g > 0.0167) over17++;
        last = t;
        if (t > end) break;
    }
    printf("  thread %d: longest stall %.1f ms, stalls longer than a frame (16.7 ms): %ld\n", id, mx * 1000, over17);
    return NULL;
}
int main(int argc, char **argv) {
    int n = argc > 1 ? atoi(argv[1]) : 4;
    secs = argc > 2 ? atof(argv[2]) : 10;
    pthread_t t[64];
    printf("host scheduling jitter, %d threads, %.0f s:\n", n, secs);
    for (long i = 0; i < n && i < 64; i++) pthread_create(&t[i], NULL, run, (void *)i);
    for (int i = 0; i < n && i < 64; i++) pthread_join(t[i], NULL);
    return 0;
}
