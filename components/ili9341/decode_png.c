#include <stdio.h>
#include "decode_png.h"
#include "pngle.h"
#include "string.h"
#include "esp_log.h"

void png_init(pngle_t *pngle, uint32_t w, uint32_t h)
{
	//ESP_LOGD(__FUNCTION__, "png_init w=%d h=%d", w, h);
	//ESP_LOGD(__FUNCTION__, "screenWidth=%d screenHeight=%d", pngle->screenWidth, pngle->screenHeight);
	pngle->imageWidth = w;
	pngle->imageHeight = h;
	pngle->reduction = false;
	pngle->scale_factor = 1.0;

	// Calculate Reduction
	if (pngle->screenWidth < pngle->imageWidth || pngle->screenHeight < pngle->imageHeight)
	{
		pngle->reduction = true;
		double factorWidth = (double) pngle->screenWidth / (double) pngle->imageWidth;
		double factorHeight = (double) pngle->screenHeight / (double) pngle->imageHeight;
		pngle->scale_factor = factorWidth;
		if (factorHeight < factorWidth)
			pngle->scale_factor = factorHeight;
		pngle->imageWidth = pngle->imageWidth * pngle->scale_factor;
		pngle->imageHeight = pngle->imageHeight * pngle->scale_factor;
	}
	//ESP_LOGD(__FUNCTION__, "reduction=%d scale_factor=%f", pngle->reduction, pngle->scale_factor);
	//ESP_LOGD(__FUNCTION__, "imageWidth=%d imageHeight=%d", pngle->imageWidth, pngle->imageHeight);

}

#define rgb565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))

void png_draw(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4])
{
	//ESP_LOGD(__FUNCTION__, "png_draw x=%d y=%d w=%d h=%d", x, y, w, h);
#if 0
	uint8_t r = rgba[0];
	uint8_t g = rgba[1];
	uint8_t b = rgba[2];
#endif

	// image reduction
	uint32_t _x = x;
	uint32_t _y = y;
	if (pngle->reduction)
	{
		_x = x * pngle->scale_factor;
		_y = y * pngle->scale_factor;
	}
	if (_y < pngle->screenHeight && _x < pngle->screenWidth)
	{
#if 0
		pngle->pixels[_y][_x].red = rgba[0];
		pngle->pixels[_y][_x].green = rgba[1];
		pngle->pixels[_y][_x].blue = rgba[2];
#endif
		pngle->pixels[_y][_x] = rgb565(rgba[0], rgba[1], rgba[2]);
	}

}

void png_finish(pngle_t *pngle)
{
	//ESP_LOGD(__FUNCTION__, "png_finish");
}

int load_png(int _x, int _y, char *file, int scr_width, int scr_height)
{
	// open PNG file
	FILE *fp = fopen(file, "rb");
	if (fp == NULL)
	{
		ESP_LOGW(__FUNCTION__, "File not found [%s]", file);
		return 0; // error
	}

	char buf[1024];
	size_t remain = 0;
	int len;

	pngle_t *pngle = pngle_new(scr_width, scr_height);

	pngle_set_init_callback(pngle, png_init);
	pngle_set_draw_callback(pngle, png_draw);
	pngle_set_done_callback(pngle, png_finish);

	double display_gamma = 2.2;
	pngle_set_display_gamma(pngle, display_gamma);

	while (!feof(fp))
	{
		if (remain >= sizeof(buf))
		{
			ESP_LOGE(__FUNCTION__, "Buffer exceeded");
			return 0; // error
		}

		len = fread(buf + remain, 1, sizeof(buf) - remain, fp);
		if (len <= 0)
			break;

		int fed = pngle_feed(pngle, buf, remain + len);
		if (fed < 0)
		{
			ESP_LOGE(__FUNCTION__, "ERROR; %s", pngle_error(pngle));
			return 0; // error
		}

		remain = remain + len - fed;
		if (remain > 0)
			memmove(buf, buf + fed, remain);
	}

	fclose(fp);

	uint16_t pngWidth = scr_width;
	uint16_t offsetX = 0;
	if (scr_width > pngle->imageWidth)
	{
		pngWidth = pngle->imageWidth;
		offsetX = (scr_width - pngle->imageWidth) / 2;
	}
	//ESP_LOGD(__FUNCTION__, "pngWidth=%d offsetX=%d", pngWidth, offsetX);

	uint16_t pngHeight = scr_height;
	uint16_t offsetY = 0;
	if (scr_height > pngle->imageHeight)
	{
		pngHeight = pngle->imageHeight;
		offsetY = (scr_height - pngle->imageHeight) / 2;
	}
	//ESP_LOGD(__FUNCTION__, "pngHeight=%d offsetY=%d", pngHeight, offsetY);
	uint16_t *colors = (uint16_t*) malloc(sizeof(uint16_t) * pngWidth);

#if 0
	for(int y = 0; y < pngHeight; y++){
		for(int x = 0;x < pngWidth; x++){
			pixel_png pixel = pngle->pixels[y][x];
			uint16_t color = rgb565_conv(pixel.red, pixel.green, pixel.blue);
			lcdDrawPixel(dev, x+offsetX, y+offsetY, color);
		}
	}
#endif

	for (int y = 0; y < pngHeight; y++)
	{
		for (int x = 0; x < pngWidth; x++)
		{
			//pixel_png pixel = pngle->pixels[y][x];
			//colors[x] = rgb565_conv(pixel.red, pixel.green, pixel.blue);
			colors[x] = pngle->pixels[y][x];
		}
		lcdDrawMultiPixels(offsetX, y + offsetY, pngWidth, colors);
		//vTaskDelay(1);
	}
	free(colors);
	pngle_destroy(pngle, scr_width, scr_height);
	return 1;
}
