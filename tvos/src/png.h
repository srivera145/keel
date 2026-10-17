#pragma once
#include "core.h"
/* Write an XRGB pixel buffer as a 24-bit PNG. Returns 1 on success. */
int png_write(const char *path, const uint32_t *px, int w, int h, int stride);
