/* KEEL OS — render thread pool and background job queue. */
#pragma once

typedef void (*ParFn)(void *ctx, int index, int thread);
typedef void (*JobFn)(void *arg);

void jobs_init(int render_threads, int bg_threads);
void jobs_shutdown(void);
int jobs_render_threads(void);
/* Run fn(ctx, i) for i in [0,n) across the render pool; returns when all done. */
void parallel_for(int n, ParFn fn, void *ctx);
/* Queue low-priority background work (asset generation). */
void job_submit(JobFn fn, void *arg, int priority);
int jobs_pending(void);
