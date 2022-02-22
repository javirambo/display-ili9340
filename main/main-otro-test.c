#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "ili9341.h"

#define	INTERVAL		140
#define WAIT	vTaskDelay(INTERVAL)

// FONTS (indices del init_fonts)
#define LATIN32B 0
#define ILGH24XB 1

static const char *TAG = "main";

TickType_t HorizontalTest()
{
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	lcdClear(BLACK);
	char ascii[20];

	strcpy(ascii, "Direction=0");
	lcdSetFontDirection(DIRECTION0);

	FontxFile fx;
	lcdSetFontEx(LATIN32B, &fx);
	lcdSetFontColor(RED);

	int y = 0;
	y += fx.height;
	int x = lcdDrawString(0, y, "linea numero 1");
	lcdDrawString(x, y, "..."); // x queda apuntando al sig char
	y += fx.height;
	lcdDrawString(0, y, "linea numero 2");
	y += fx.height;
	lcdDrawString(0, y, "linea numero 3");
	y += fx.height;
	lcdDrawString(0, y, "linea numero 4");

	/*lcdPrintf(0, fx.height - 1, "%s", "X123");
	 lcdSetFontUnderLine(RED);
	 lcdPrintf(0, fx.height * 2 - 1, "%s", ascii);
	 lcdUnsetFontUnderLine();

	 lcdSetFontFill( GREEN);
	 lcdPrintf(0, fx.height * 3 - 1, "%s", ascii);
	 lcdSetFontUnderLine( RED);
	 lcdPrintf(0, fx.height * 4 - 1, "%s", ascii);
	 lcdUnsetFontFill();
	 lcdUnsetFontUnderLine();*/

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%d", diffTick * portTICK_RATE_MS);
	return diffTick;
}

TickType_t JPEGTest()
{
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	lcdClear(BLUE);
	lcdLoadJpg(0, 0, "chancha-parada.jpg");

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
	// nombre del FS y cantidad max de archivos:
	lcdInitFS("spiffs", 16); 	// solo si uso los recursos

	// guardar los indices para luego setear las diferentes fonts a usar:
	lcdInitFonts(2, "LATIN32B.FNT", "ILGH24XB.FNT");	// solo si uso las fonts

	lcdInitDisplay();	// siempre y solo para ILI9341

	xTaskCreate(ILI9341, "ILI9341", 1024 * 6, NULL, 2, NULL);
}
