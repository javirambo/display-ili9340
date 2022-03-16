#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "ili9341.h"
#include "fsTools.h"

#define	INTERVAL		140
#define WAIT	vTaskDelay(INTERVAL)

// FONTS (indices del init_fonts)
#define LATIN32B 0
#define ILGH24XB 1

//static const char *TAG = "main";

TickType_t ScrollTest()
{
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	lcdClearScreen(BLACK);
	FontxFile fx;
	lcdSetFontEx(ILGH24XB,&fx);
	lcdSetFontColor(RED);
	lcdSetCursor(0, 0);


	//lcdGetFont(&fx);
	ESP_LOGD(__FUNCTION__, "fontWidth=%d fontHeight=%d", fx.width, fx.height);

	//uint16_t color;
	char ascii[30];

	int lines = (CONFIG_HEIGHT - fx.height) / fx.height;
	ESP_LOGD(__FUNCTION__, "height=%d fontHeight=%d lines=%d", CONFIG_HEIGHT, fx.height, lines);
	int ymax = (lines + 1) * fx.height;
	ESP_LOGD(__FUNCTION__, "ymax=%d", ymax);

	lcdSetFontDirection( 0);
	//lcdFillScreen( BLACK);

	strcpy((char*) ascii, "Vertical Smooth Scroll");
	lcdPrintAtPos(0, fx.height - 1, ascii);

	//color = CYAN;
	uint16_t vsp = fx.height * 2;
	uint16_t ypos = fx.height * 2 - 1;
	//for(int i=0;i<30;i++) {
	for (int i = 0; i < lines + 10; i++)
	{
		ESP_LOGD(__FUNCTION__, "i=%d ypos=%d", i, ypos);
		sprintf((char*) ascii, "This is text line %d", i);
		if (i < lines)
		{
			lcdPrintAtPos( 0, ypos, ascii);
		}
		else
		{
			lcdDrawFillRect( 0, ypos - fx.height, CONFIG_HEIGHT - 1, ypos, BLACK);
			lcdSetScrollArea( fx.height, (CONFIG_HEIGHT - fx.height), 0);
			lcdScroll( vsp);
			vsp = vsp + fx.height;
			if (vsp > ymax)
				vsp = fx.height * 2;
			lcdPrintAtPos( 0, ypos, ascii);
		}
		ypos = ypos + fx.height;
		if (ypos > ymax)
			ypos = fx.height * 2 - 1;
		//vTaskDelay(25);
	}

	// Initialize scroll area
	//lcdSetScrollArea(dev, 0, 0x0140, 0);

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%d", diffTick*portTICK_RATE_MS);
	return diffTick;
}

void ScrollReset()
{
	//lcdResetScrollArea(dev, 320);
	//lcdResetScrollArea(dev, 240);
	lcdResetScrollArea(CONFIG_HEIGHT);
	lcdScroll(0);
}

TickType_t HorizontalTest()
{
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	lcdClearScreen(BLACK);
	char ascii[20];

	strcpy(ascii, "Direction=0");
	lcdSetFontDirection(DIRECTION0);

	lcdSetFont(LATIN32B);
	lcdSetFontColor(RED);

	lcdPrintf("%s\n", "linea numero 1");
	lcdPrintf("%s\n", "linea numero 2");

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%d", diffTick * portTICK_RATE_MS);
	return diffTick;
}

TickType_t JPEGTest()
{
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	lcdClearScreen(BLUE);
	lcdLoadJpg(-1, -1, "chancha-parada.jpg");

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%d", diffTick * portTICK_RATE_MS);
	return diffTick;
}

void ILI9341(void *pvParameters)
{
	while (1)
	{
		HorizontalTest();
		WAIT;

		//JPEGTest();
		//WAIT;

		ScrollTest();
		WAIT;
		ScrollReset();
	}
}

void app_main(void)
{
	fs_init();
	// guardar los indices para luego setear las diferentes fonts a usar:
	lcdInitFonts(2, "LATIN32B.FNT", "ILGH24XB.FNT");	// solo si uso las fonts
	lcdInitDisplay();	// siempre y solo para ILI9341
	lcdClearScreen(BLACK);
	lcdSetFontDirection(DIRECTION0);

	xTaskCreate(ILI9341, "ILI9341", 1024 * 6, NULL, 2, NULL);
}
