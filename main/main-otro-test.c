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
	}
}

void app_main(void)
{
	// guardar los indices para luego setear las diferentes fonts a usar:
	lcdInitFonts(2, "LATIN32B.FNT", "ILGH24XB.FNT");	// solo si uso las fonts
	lcdInitDisplay();	// siempre y solo para ILI9341
	lcdClearScreen(BLACK);
	lcdSetFontDirection(DIRECTION0);

	xTaskCreate(ILI9341, "ILI9341", 1024 * 6, NULL, 2, NULL);
}
