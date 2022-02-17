#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "esp_heap_caps.h"

#include "ili9340.h"
#include "fontx.h"
#include "bmpfile.h"
#include "decode_jpeg.h"
#include "decode_png.h"
#include "pngle.h"

#include "driver/gpio.h"

#define	INTERVAL		140
#define WAIT	vTaskDelay(INTERVAL)

static const char *TAG = "ILI9340";

TFT_t dev;
FontxFile fx16G[2];
FontxFile fx24G[2];
FontxFile fx32G[2];
FontxFile fx32L[2];
FontxFile fx16M[2];
FontxFile fx24M[2];
FontxFile fx32M[2];
FontxFile fxTEST[2]; // Upheaval.fon

static void SPIFFS_Directory(char *path)
{
	DIR *dir = opendir(path);
	assert(dir != NULL);
	while (true)
	{
		struct dirent *pe = readdir(dir);
		if (!pe)
			break;
		ESP_LOGI(__FUNCTION__, "d_name=%s d_ino=%d d_type=%x", pe->d_name, pe->d_ino, pe->d_type);
	}
	closedir(dir);
}

TickType_t FillTest(TFT_t *dev, int width, int height)
{
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	lcdFillScreen(dev, RED);
	vTaskDelay(50);
	lcdFillScreen(dev, GREEN);
	vTaskDelay(50);
	lcdFillScreen(dev, BLUE);
	vTaskDelay(50);

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%d", diffTick*portTICK_RATE_MS);
	return diffTick;
}

TickType_t ColorBarTest(TFT_t *dev, int width, int height)
{
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	if (width < height)
	{
		uint16_t y1, y2;
		y1 = height / 3;
		y2 = (height / 3) * 2;
		lcdDrawFillRect(dev, 0, 0, width - 1, y1 - 1, RED);
		vTaskDelay(1);
		lcdDrawFillRect(dev, 0, y1 - 1, width - 1, y2 - 1, GREEN);
		vTaskDelay(1);
		lcdDrawFillRect(dev, 0, y2 - 1, width - 1, height - 1, BLUE);
	}
	else
	{
		uint16_t x1, x2;
		x1 = width / 3;
		x2 = (width / 3) * 2;
		lcdDrawFillRect(dev, 0, 0, x1 - 1, height - 1, RED);
		vTaskDelay(1);
		lcdDrawFillRect(dev, x1 - 1, 0, x2 - 1, height - 1, GREEN);
		vTaskDelay(1);
		lcdDrawFillRect(dev, x2 - 1, 0, width - 1, height - 1, BLUE);
	}

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%d", diffTick*portTICK_RATE_MS);
	return diffTick;
}

TickType_t ArrowTest(TFT_t *dev, FontxFile *fx, int width, int height)
{
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	// get font width & height
	uint8_t buffer[FontxGlyphBufSize];
	uint8_t fontWidth;
	uint8_t fontHeight;
	GetFontx(fx, 0, buffer, &fontWidth, &fontHeight);
	ESP_LOGD(__FUNCTION__, "fontWidth=%d fontHeight=%d", fontWidth, fontHeight);

	uint16_t xpos;
	uint16_t ypos;
	int stlen;
	uint8_t ascii[24];
	uint16_t color;

	lcdFillScreen(dev, BLACK);

	strcpy((char*) ascii, "ILI9341");

	if (width < height)
	{
		xpos = ((width - fontHeight) / 2) - 1;
		ypos = (height - (strlen((char*) ascii) * fontWidth)) / 2;
		lcdSetFontDirection(dev, DIRECTION90);
	}
	else
	{
		ypos = ((height - fontHeight) / 2) - 1;
		xpos = (width - (strlen((char*) ascii) * fontWidth)) / 2;
		lcdSetFontDirection(dev, DIRECTION0);
	}
	color = WHITE;
	lcdDrawString(dev, fx, xpos, ypos, ascii, color);

	lcdSetFontDirection(dev, 0);
	//lcdFillScreen(dev, WHITE);
	color = RED;
	lcdDrawFillArrow(dev, 10, 10, 0, 0, 5, color);
	strcpy((char*) ascii, "0,0");
	lcdDrawString(dev, fx, 0, 30, ascii, color);

	color = GREEN;
	lcdDrawFillArrow(dev, width - 11, 10, width - 1, 0, 5, color);
	//strcpy((char *)ascii, "79,0");
	sprintf((char*) ascii, "%d,0", width - 1);
	stlen = strlen((char*) ascii);
	xpos = (width - 1) - (fontWidth * stlen);
	lcdDrawString(dev, fx, xpos, 30, ascii, color);

	color = GRAY;
	lcdDrawFillArrow(dev, 10, height - 11, 0, height - 1, 5, color);
	//strcpy((char *)ascii, "0,159");
	sprintf((char*) ascii, "0,%d", height - 1);
	ypos = (height - 11) - (fontHeight) + 5;
	lcdDrawString(dev, fx, 0, ypos, ascii, color);

	color = CYAN;
	lcdDrawFillArrow(dev, width - 11, height - 11, width - 1, height - 1, 5, color);
	//strcpy((char *)ascii, "79,159");
	sprintf((char*) ascii, "%d,%d", width - 1, height - 1);
	stlen = strlen((char*) ascii);
	xpos = (width - 1) - (fontWidth * stlen);
	lcdDrawString(dev, fx, xpos, ypos, ascii, color);

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%d", diffTick*portTICK_RATE_MS);
	return diffTick;
}

TickType_t DirectionTest(TFT_t *dev, FontxFile *fx, int width, int height)
{
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	// get font width & height
	uint8_t buffer[FontxGlyphBufSize];
	uint8_t fontWidth;
	uint8_t fontHeight;
	GetFontx(fx, 0, buffer, &fontWidth, &fontHeight);
	//ESP_LOGI(__FUNCTION__,"fontWidth=%d fontHeight=%d",fontWidth,fontHeight);

	uint16_t color;
	lcdFillScreen(dev, BLACK);
	uint8_t ascii[20];

	color = RED;
	strcpy((char*) ascii, "Direction=0");
	lcdSetFontDirection(dev, 0);
	lcdDrawString(dev, fx, 0, fontHeight - 1, ascii, color);

	color = BLUE;
	strcpy((char*) ascii, "Direction=2");
	lcdSetFontDirection(dev, 2);
	lcdDrawString(dev, fx, (width - 1), (height - 1) - (fontHeight * 1), ascii, color);

	color = CYAN;
	strcpy((char*) ascii, "Direction=1");
	lcdSetFontDirection(dev, 1);
	lcdDrawString(dev, fx, (width - 1) - fontHeight, 0, ascii, color);

	color = GREEN;
	strcpy((char*) ascii, "Direction=3");
	lcdSetFontDirection(dev, 3);
	lcdDrawString(dev, fx, (fontHeight - 1), height - 1, ascii, color);

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%d", diffTick*portTICK_RATE_MS);
	return diffTick;
}

TickType_t HorizontalTest(TFT_t *dev, FontxFile *fx, int width, int height)
{
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	// get font width & height
	uint8_t buffer[FontxGlyphBufSize];
	uint8_t fontWidth;
	uint8_t fontHeight;
	GetFontx(fx, 0, buffer, &fontWidth, &fontHeight);
	//ESP_LOGI(__FUNCTION__,"fontWidth=%d fontHeight=%d",fontWidth,fontHeight);

	uint16_t color;
	lcdFillScreen(dev, BLACK);
	uint8_t ascii[20];

	color = RED;
	strcpy((char*) ascii, "Direction=0");
	lcdSetFontDirection(dev, 0);
	lcdDrawString(dev, fx, 0, fontHeight * 1 - 1, ascii, color);
	lcdSetFontUnderLine(dev, RED);
	lcdDrawString(dev, fx, 0, fontHeight * 2 - 1, ascii, color);
	lcdUnsetFontUnderLine(dev);

	lcdSetFontFill(dev, GREEN);
	lcdDrawString(dev, fx, 0, fontHeight * 3 - 1, ascii, color);
	lcdSetFontUnderLine(dev, RED);
	lcdDrawString(dev, fx, 0, fontHeight * 4 - 1, ascii, color);
	lcdUnsetFontFill(dev);
	lcdUnsetFontUnderLine(dev);

	color = BLUE;
	strcpy((char*) ascii, "Direction=2");
	lcdSetFontDirection(dev, 2);
	lcdDrawString(dev, fx, width - 1, height - (fontHeight * 1) - 1, ascii, color);
	lcdSetFontUnderLine(dev, BLUE);
	lcdDrawString(dev, fx, width - 1, height - (fontHeight * 2) - 1, ascii, color);
	lcdUnsetFontUnderLine(dev);

	lcdSetFontFill(dev, YELLOW);
	lcdDrawString(dev, fx, width - 1, height - (fontHeight * 3) - 1, ascii, color);
	lcdSetFontUnderLine(dev, BLUE);
	lcdDrawString(dev, fx, width - 1, height - (fontHeight * 4) - 1, ascii, color);
	lcdUnsetFontFill(dev);
	lcdUnsetFontUnderLine(dev);

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%d", diffTick*portTICK_RATE_MS);
	return diffTick;
}

TickType_t VerticalTest(TFT_t *dev, FontxFile *fx, int width, int height)
{
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	// get font width & height
	uint8_t buffer[FontxGlyphBufSize];
	uint8_t fontWidth;
	uint8_t fontHeight;
	GetFontx(fx, 0, buffer, &fontWidth, &fontHeight);
	//ESP_LOGI(__FUNCTION__,"fontWidth=%d fontHeight=%d",fontWidth,fontHeight);

	uint16_t color;
	lcdFillScreen(dev, BLACK);
	uint8_t ascii[20];

	color = RED;
	strcpy((char*) ascii, "Direction=1");
	lcdSetFontDirection(dev, 1);
	lcdDrawString(dev, fx, width - (fontHeight * 1), 0, ascii, color);
	lcdSetFontUnderLine(dev, RED);
	lcdDrawString(dev, fx, width - (fontHeight * 2), 0, ascii, color);
	lcdUnsetFontUnderLine(dev);

	lcdSetFontFill(dev, GREEN);
	lcdDrawString(dev, fx, width - (fontHeight * 3), 0, ascii, color);
	lcdSetFontUnderLine(dev, RED);
	lcdDrawString(dev, fx, width - (fontHeight * 4), 0, ascii, color);
	lcdUnsetFontFill(dev);
	lcdUnsetFontUnderLine(dev);

	color = BLUE;
	strcpy((char*) ascii, "Direction=3");
	lcdSetFontDirection(dev, 3);
	lcdDrawString(dev, fx, (fontHeight * 1) - 1, height - 1, ascii, color);
	lcdSetFontUnderLine(dev, BLUE);
	lcdDrawString(dev, fx, (fontHeight * 2) - 1, height - 1, ascii, color);
	lcdUnsetFontUnderLine(dev);

	lcdSetFontFill(dev, YELLOW);
	lcdDrawString(dev, fx, (fontHeight * 3) - 1, height - 1, ascii, color);
	lcdSetFontUnderLine(dev, BLUE);
	lcdDrawString(dev, fx, (fontHeight * 4) - 1, height - 1, ascii, color);
	lcdUnsetFontFill(dev);
	lcdUnsetFontUnderLine(dev);

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%d", diffTick*portTICK_RATE_MS);
	return diffTick;
}

TickType_t LineTest(TFT_t *dev, int width, int height)
{
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	uint16_t color;
	//lcdFillScreen(dev, WHITE);
	lcdFillScreen(dev, BLACK);
	color = RED;
	for (int ypos = 0; ypos < height; ypos = ypos + 10)
	{
		lcdDrawLine(dev, 0, ypos, width, ypos, color);
	}

	for (int xpos = 0; xpos < width; xpos = xpos + 10)
	{
		lcdDrawLine(dev, xpos, 0, xpos, height, color);
	}

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%d", diffTick*portTICK_RATE_MS);
	return diffTick;
}

TickType_t CircleTest(TFT_t *dev, int width, int height)
{
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	uint16_t color;
	//lcdFillScreen(dev, WHITE);
	lcdFillScreen(dev, BLACK);
	color = GRAY;
	uint16_t xpos = width / 2;
	uint16_t ypos = height / 2;
	for (int i = 5; i < height; i = i + 5)
	{
		lcdDrawCircle(dev, xpos, ypos, i, color);
	}

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%d", diffTick*portTICK_RATE_MS);
	return diffTick;
}

TickType_t RectAngleTest(TFT_t *dev, int width, int height)
{
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	uint16_t color;
	//lcdFillScreen(dev, WHITE);
	lcdFillScreen(dev, BLACK);
	color = CYAN;
	uint16_t xpos = width / 2;
	uint16_t ypos = height / 2;

	uint16_t w = width * 0.6;
	uint16_t h = w * 0.5;
	int angle;
	for (angle = 0; angle <= (360 * 3); angle = angle + 30)
	{
		lcdDrawRectAngle(dev, xpos, ypos, w, h, angle, color);
		usleep(10000);
		lcdDrawRectAngle(dev, xpos, ypos, w, h, angle, BLACK);
	}

	for (angle = 0; angle <= 180; angle = angle + 30)
	{
		lcdDrawRectAngle(dev, xpos, ypos, w, h, angle, color);
	}

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%d", diffTick*portTICK_RATE_MS);
	return diffTick;
}

TickType_t TriangleTest(TFT_t *dev, int width, int height)
{
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	uint16_t color;
	//lcdFillScreen(dev, WHITE);
	lcdFillScreen(dev, BLACK);
	color = CYAN;
	uint16_t xpos = width / 2;
	uint16_t ypos = height / 2;

	uint16_t w;
	uint16_t h;
	if (height > width)
	{
		w = width * 0.6;
		h = w * 1.0;
	}
	else
	{
		w = height * 0.6;
		h = w * 1.0;
	}
	int angle;

	for (angle = 0; angle <= (360 * 3); angle = angle + 30)
	{
		lcdDrawTriangle(dev, xpos, ypos, w, h, angle, color);
		usleep(10000);
		lcdDrawTriangle(dev, xpos, ypos, w, h, angle, BLACK);
	}

	for (angle = 0; angle <= 360; angle = angle + 30)
	{
		lcdDrawTriangle(dev, xpos, ypos, w, h, angle, color);
	}

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%d", diffTick*portTICK_RATE_MS);
	return diffTick;
}

TickType_t RoundRectTest(TFT_t *dev, int width, int height)
{
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	uint16_t color;
	uint16_t limit = width;
	if (width > height)
		limit = height;
	//lcdFillScreen(dev, WHITE);
	lcdFillScreen(dev, BLACK);
	color = BLUE;
	for (int i = 5; i < limit; i = i + 5)
	{
		if (i > (limit - i - 1))
			break;
		//ESP_LOGI(__FUNCTION__, "i=%d, width-i-1=%d",i, width-i-1);
		lcdDrawRoundRect(dev, i, i, (width - i - 1), (height - i - 1), 10, color);
	}

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%d", diffTick*portTICK_RATE_MS);
	return diffTick;
}

TickType_t FillRectTest(TFT_t *dev, int width, int height)
{
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	uint16_t color;
	lcdFillScreen(dev, CYAN);

	uint16_t red;
	uint16_t green;
	uint16_t blue;
	srand((unsigned int) time( NULL));
	for (int i = 1; i < 100; i++)
	{
		red = rand() % 255;
		green = rand() % 255;
		blue = rand() % 255;
		color = rgb565_conv(red, green, blue);
		uint16_t xpos = rand() % width;
		uint16_t ypos = rand() % height;
		uint16_t size = rand() % (width / 5);
		lcdDrawFillRect(dev, xpos, ypos, xpos + size, ypos + size, color);
	}

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%d", diffTick*portTICK_RATE_MS);
	return diffTick;
}

TickType_t ColorTest(TFT_t *dev, int width, int height)
{
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	uint16_t color;
	lcdFillScreen(dev, WHITE);
	color = RED;
	uint16_t delta = height / 16;
	uint16_t ypos = 0;
	for (int i = 0; i < 16; i++)
	{
		//ESP_LOGI(__FUNCTION__, "color=0x%x",color);
		lcdDrawFillRect(dev, 0, ypos, width - 1, ypos + delta, color);
		color = color >> 1;
		ypos = ypos + delta;
	}

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%d", diffTick*portTICK_RATE_MS);
	return diffTick;
}

TickType_t ScrollTest(TFT_t *dev, FontxFile *fx, int width, int height)
{
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	// get font width & height
	uint8_t buffer[FontxGlyphBufSize];
	uint8_t fontWidth;
	uint8_t fontHeight;
	GetFontx(fx, 0, buffer, &fontWidth, &fontHeight);
	ESP_LOGD(__FUNCTION__, "fontWidth=%d fontHeight=%d", fontWidth, fontHeight);

	uint16_t color;
	uint8_t ascii[30];

	int lines = (height - fontHeight) / fontHeight;
	ESP_LOGD(__FUNCTION__, "height=%d fontHeight=%d lines=%d", height, fontHeight, lines);
	int ymax = (lines + 1) * fontHeight;
	ESP_LOGD(__FUNCTION__, "ymax=%d", ymax);

	lcdSetFontDirection(dev, 0);
	lcdFillScreen(dev, BLACK);

	strcpy((char*) ascii, "Vertical Smooth Scroll");
	lcdDrawString(dev, fx, 0, fontHeight - 1, ascii, RED);

	color = CYAN;
	uint16_t vsp = fontHeight * 2;
	uint16_t ypos = fontHeight * 2 - 1;
	//for(int i=0;i<30;i++) {
	for (int i = 0; i < lines + 10; i++)
	{
		ESP_LOGD(__FUNCTION__, "i=%d ypos=%d", i, ypos);
		sprintf((char*) ascii, "This is text line %d", i);
		if (i < lines)
		{
			lcdDrawString(dev, fx, 0, ypos, ascii, color);
		}
		else
		{
			lcdDrawFillRect(dev, 0, ypos - fontHeight, width - 1, ypos, BLACK);
			lcdSetScrollArea(dev, fontHeight, (height - fontHeight), 0);
			lcdScroll(dev, vsp);
			vsp = vsp + fontHeight;
			if (vsp > ymax)
				vsp = fontHeight * 2;
			lcdDrawString(dev, fx, 0, ypos, ascii, color);
		}
		ypos = ypos + fontHeight;
		if (ypos > ymax)
			ypos = fontHeight * 2 - 1;
		vTaskDelay(25);
	}

	// Initialize scroll area
	//lcdSetScrollArea(dev, 0, 0x0140, 0);

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%d", diffTick*portTICK_RATE_MS);
	return diffTick;
}

void ScrollReset(TFT_t *dev, int width, int height)
{
	//lcdResetScrollArea(dev, 320);
	//lcdResetScrollArea(dev, 240);
	lcdResetScrollArea(dev, height);
	lcdScroll(dev, 0);
}

TickType_t BMPTest(TFT_t *dev, char *file, int width, int height)
{
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	load_bmp(dev, 0, 0, file, width, height);

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%d", diffTick*portTICK_RATE_MS);
	return diffTick;
}

TickType_t JPEGTest(TFT_t *dev, char *file, int width, int height)
{
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	load_jpg(dev, 0, 0, file, width, height);

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%d", diffTick*portTICK_RATE_MS);
	return diffTick;
}

TickType_t PNGTest(TFT_t *dev, char *file, int width, int height)
{
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	load_png(dev, 0, 0, file, width, height);

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%d", diffTick*portTICK_RATE_MS);
	return diffTick;
}

TickType_t CodeTest(TFT_t *dev, FontxFile *fx, int width, int height, uint16_t start, uint16_t end)
{
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	// get font width & height
	uint8_t buffer[FontxGlyphBufSize];
	uint8_t fontWidth;
	uint8_t fontHeight;
	GetFontx(fx, 0, buffer, &fontWidth, &fontHeight);
	//ESP_LOGI(__FUNCTION__,"fontWidth=%d fontHeight=%d",fontWidth,fontHeight);
	uint8_t xmoji = width / fontWidth;
	uint8_t ymoji = height / fontHeight;
	//ESP_LOGI(__FUNCTION__,"xmoji=%d ymoji=%d",xmoji, ymoji);

	uint16_t color;
	lcdFillScreen(dev, BLACK);
	uint8_t code;

	color = CYAN;
	lcdSetFontDirection(dev, 0);
	//code = 0xA0;
	code = start;
	for (int y = 0; y < ymoji; y++)
	{
		uint16_t xpos = 0;
		uint16_t ypos = fontHeight * (y + 1) - 1;
		for (int x = 0; x < xmoji; x++)
		{
			xpos = lcdDrawCode(dev, fx, xpos, ypos, code, color);
			if (code == 0xFF)
				break;
			code++;
			if (code > end)
				break;
		}
	}

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%d", diffTick*portTICK_RATE_MS);
	return diffTick;
}

static void init_fonts()
{
	InitFontx(fx16G, "/spiffs/ILGH16XB.FNT", ""); // 8x16Dot Gothic
	InitFontx(fx24G, "/spiffs/ILGH24XB.FNT", ""); // 12x24Dot Gothic
	InitFontx(fx32G, "/spiffs/ILGH32XB.FNT", ""); // 16x32Dot Gothic
	InitFontx(fx32L, "/spiffs/LATIN32B.FNT", ""); // 16x32Dot Latinc

	InitFontx(fx16M, "/spiffs/ILMH16XB.FNT", ""); // 8x16Dot Mincyo
	InitFontx(fx24M, "/spiffs/ILMH24XB.FNT", ""); // 12x24Dot Mincyo
	InitFontx(fx32M, "/spiffs/ILMH32XB.FNT", ""); // 16x32Dot Mincyo

	InitFontx(fxTEST, "/spiffs/font9x18B.fnt", "");
}

static void init_display()
{
	spi_master_init(&dev, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_TFT_CS_GPIO, CONFIG_DC_GPIO, CONFIG_RESET_GPIO, CONFIG_BL_GPIO, -1, -1, -1);

	lcdInit(&dev, 0x9341, CONFIG_WIDTH, CONFIG_HEIGHT, CONFIG_OFFSETX, CONFIG_OFFSETY);

#if CONFIG_INVERSION
	ESP_LOGI(TAG, "Enable Display Inversion");
	lcdInversionOn(&dev);
#endif

#if CONFIG_RGB_COLOR
	ESP_LOGI(TAG, "Change BGR filter to RGB filter");
	lcdBGRFilter(&dev);
#endif
}

static void init_spiffs()
{
	ESP_LOGI(TAG, "Initializing SPIFFS");

	esp_vfs_spiffs_conf_t conf = {
		.base_path = "/spiffs",
		.partition_label = NULL,
		.max_files = 16,
		.format_if_mount_failed = true
	};

	// Use settings defined above toinitialize and mount SPIFFS filesystem.
	// Note: esp_vfs_spiffs_register is anall-in-one convenience function.
	esp_err_t ret = esp_vfs_spiffs_register(&conf);

	if (ret != ESP_OK)
	{
		if (ret == ESP_FAIL)
		{
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		}
		else if (ret == ESP_ERR_NOT_FOUND)
		{
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		}
		else
		{
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
		}
		return;
	}

	size_t total = 0, used = 0;
	ret = esp_spiffs_info(NULL, &total, &used);
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
	}
	else
	{
		ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
	}

	SPIFFS_Directory("/spiffs/");
}

void ILI9341(void *pvParameters)
{
	init_fonts();
	init_display();

	while (1)
	{

		FillTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		ColorBarTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		ArrowTest(&dev, fx16G, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		LineTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		CircleTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		RoundRectTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		RectAngleTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		TriangleTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		DirectionTest(&dev, fx24G, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		HorizontalTest(&dev, fx24G, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		VerticalTest(&dev, fx24G, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		FillRectTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		ColorTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		CodeTest(&dev, fx32G, CONFIG_WIDTH, CONFIG_HEIGHT, 0x00, 0x7f);
		WAIT;

		CodeTest(&dev, fx32G, CONFIG_WIDTH, CONFIG_HEIGHT, 0x80, 0xff);
		WAIT;

		CodeTest(&dev, fx32L, CONFIG_WIDTH, CONFIG_HEIGHT, 0x80, 0xff);
		WAIT;

		BMPTest(&dev, "/spiffs/esp32_ro.bmp", CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		JPEGTest(&dev, "/spiffs/esp32.jpeg", CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		PNGTest(&dev, "/spiffs/esp_logo.png", CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		ScrollTest(&dev, fx16G, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;
		ScrollReset(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);

		//------------------------------------------------------------------------------
		// OJO con esto: los x / y estan para el orto porque el display lo toma invertido
		// TODO hacer un wrapper para que use el print de forma mas "normal"
		lcdFillScreen(&dev, RED);
		lcdSetFontDirection(&dev, 1);
		lcdDrawString(&dev, fx32L, 200, 0, (uint8_t*) "1", YELLOW);
		lcdDrawString(&dev, fx32L, 0, 0, (uint8_t*) "2", YELLOW);
		lcdDrawString(&dev, fx32L, 220, CONFIG_WIDTH, (uint8_t*) "3", YELLOW);
		lcdDrawString(&dev, fx32L, 0, CONFIG_WIDTH, (uint8_t*) "4", YELLOW);
		lcdDrawString(&dev, fx32L, 0, 300, (uint8_t*) "5", YELLOW);
		ESP_LOGI(TAG, "test print letras en distintas posiciones");
		WAIT;
		//------------------------------------------------------------------------------

		// Multi Font Test
		uint16_t color;
		uint8_t ascii[40];
		uint16_t margin = 10;
		lcdFillScreen(&dev, BLACK);
		color = WHITE;
		lcdSetFontDirection(&dev, 0);
		uint16_t xpos = 0;
		uint16_t ypos = 15;
		int xd = 0;
		int yd = 1;
		if (CONFIG_WIDTH < CONFIG_HEIGHT)
		{
			lcdSetFontDirection(&dev, 1);
			xpos = (CONFIG_WIDTH - 1) - 16;
			ypos = 0;
			xd = 1;
			yd = 0;
		}
		strcpy((char*) ascii, "16Dot Gothic Font");
		lcdDrawString(&dev, fx16G, xpos, ypos, ascii, color);

		xpos = xpos - (24 * xd) - (margin * xd);
		ypos = ypos + (16 * yd) + (margin * yd);
		strcpy((char*) ascii, "24Dot Gothic Font");
		lcdDrawString(&dev, fx24G, xpos, ypos, ascii, color);

		xpos = xpos - (32 * xd) - (margin * xd);
		ypos = ypos + (24 * yd) + (margin * yd);
		strcpy((char*) ascii, "32Dot Gothic Font");
		lcdDrawString(&dev, fx32G, xpos, ypos, ascii, color);
		xpos = xpos - (32 * xd) - (margin * xd);
		ypos = ypos + (32 * yd) + (margin * yd);
		strcpy((char*) ascii, "Latin32B 0123456");
		lcdDrawString(&dev, fx32L, xpos, ypos, ascii, color);

		xpos = xpos - (10 * xd) - (margin * xd);
		ypos = ypos + (10 * yd) + (margin * yd);
		strcpy((char*) ascii, "16Dot Mincyo Font");
		lcdDrawString(&dev, fx16M, xpos, ypos, ascii, color);

		xpos = xpos - (24 * xd) - (margin * xd);
		ypos = ypos + (16 * yd) + (margin * yd);
		strcpy((char*) ascii, "24Dot Mincyo Font");
		lcdDrawString(&dev, fx24M, xpos, ypos, ascii, color);

		if (CONFIG_WIDTH >= 240)
		{
			xpos = xpos - (32 * xd) - (margin * xd);
			ypos = ypos + (24 * yd) + (margin * yd);
			strcpy((char*) ascii, "32Dot Mincyo Font");
			lcdDrawString(&dev, fx32M, xpos, ypos, ascii, color);
		}
		lcdSetFontDirection(&dev, 0);
		WAIT;

	} // end while

	// never reach
	while (1)
	{
		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}
}

void app_main(void)
{
	init_spiffs();
	xTaskCreate(ILI9341, "ILI9341", 1024 * 6, NULL, 2, NULL);
}
