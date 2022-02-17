#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "pngle.h"
#include "ili9340.h"

void png_init(pngle_t *pngle, uint32_t w, uint32_t h);
void png_draw(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4]);
void png_finish(pngle_t *pngle);
int load_png(TFT_t *dev, int _x, int _y, char *file, int scr_width, int scr_height);
