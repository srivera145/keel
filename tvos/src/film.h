/* KEEL OS — real-time "film" renderer: parallax scene layers + camera + FX. */
#pragma once
#include "gfx.h"

void film_prepare(int movie);          /* generate layers in the background */
int film_ready(int movie);
/* Draw the moving picture for movie at time t into a rect (letterboxed). */
void film_draw(Gfx *g, int movie, double t, float x, float y, float w, float h, int letterbox);
