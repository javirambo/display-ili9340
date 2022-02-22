#pragma once
#include <stdint.h>
#include <stdbool.h>

#include "ili9341.h"
#include "pngle.h"

void png_init(pngle_t *pngle, uint32_t w, uint32_t h);
void png_draw(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4]);
void png_finish(pngle_t *pngle);
int load_png(int _x, int _y, char *file, int scr_width, int scr_height);
