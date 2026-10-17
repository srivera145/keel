/* KEEL OS — live channels: animated broadcasts and program schedule. */
#pragma once
#include "assets.h"
#include "gfx.h"

typedef struct { int start, dur, index; const char *title; } Program; /* minutes since midnight */

void channel_draw(Gfx *g, int ch, double t, float x, float y, float w, float h);
int schedule_at(int ch, int minute_of_day, Program *out);
void schedule_next(int ch, const Program *p, Program *out);
