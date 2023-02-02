/*
 * https://github.com/nopnop2002/esp-idf-ili9340
 *
 * Super modificado por mi.
 * 2022, Javier.
 *
 */
#ifndef MAIN_ILI9340_H_
#define MAIN_ILI9340_H_

#include <stdbool.h>
#include "driver/spi_master.h"
#include "fontx.h"
#include "bmpfile.h"
#include "decode_jpeg.h"
#include "decode_png.h"
#include "ili9341.h"
#include "pngle.h"

#define BLACK       	0x0000      /*   0,   0,   0 */
#define NAVY        	0x000F      /*   0,   0, 128 */
#define DARKGREEN   	0x03E0      /*   0, 128,   0 */
#define DARKCYAN    	0x03EF      /*   0, 128, 128 */
#define MAROON      	0x7800      /* 128,   0,   0 */
#define PURPLE      	0x780F      /* 128,   0, 128 */
#define OLIVE       	0x7BE0      /* 128, 128,   0 */
#define GRAY			0x8c51
#define LIGHTGREY  		0xC618      /* 192, 192, 192 */
#define DARKGREY    	0x7BEF      /* 128, 128, 128 */
#define BLUE        	0x001F      /*   0,   0, 255 */
#define GREEN       	0x07E0      /*   0, 255,   0 */
#define CYAN        	0x07FF      /*   0, 255, 255 */
#define RED         	0xF800      /* 255,   0,   0 */
#define MAGENTA     	0xF81F      /* 255,   0, 255 */
#define YELLOW      	0xFFE0      /* 255, 255,   0 */
#define WHITE       	0xFFFF      /* 255, 255, 255 */
#define ORANGE      	0xFD20     /* 255, 165,   0 */
#define GREENYELLOW 	0xAFE5     /* 173, 255,  47 */
#define PINK        	0xF81F

#define rgb565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))

typedef enum
{
	DIRECTION0,
	DIRECTION90,
	DIRECTION180,
	DIRECTION270
} Direction_t;

typedef struct
{
	FontxFile *fx;				// font ultima seteada
	Direction_t direction;
	bool is_fill;
	bool is_underline;
	uint16_t color;				// color de letra
	uint16_t bg_color;		// color de fondo
	uint16_t ul_color;			// color del subrayado
	uint16_t y; 					// para hacer logs (usar \n etc)
	uint16_t x;
} Font_t;

typedef struct
{
	uint16_t width; 	// ancho y alto del display (solo se cambia con la configuracion sdkconfig)
	uint16_t height;
	Font_t font;		// font ultima usada
	uint16_t bg_color;	// ultimos colores usados (no para font)
	uint16_t fg_color;

	int16_t _dc;		// para indicar al spi data/command
	int16_t _bl;		// backlight

	int16_t _irq;		// --- para el touch ---
	spi_device_handle_t _TFT_Handle;
	spi_device_handle_t _XPT_Handle;
	bool _calibration;
	int16_t _min_xp; 		// Minimum xp calibration
	int16_t _min_yp; 		// Minimum yp calibration
	int16_t _max_xp; 		// Maximum xp calibration
	int16_t _max_yp; 		// Maximum yp calibration
	int16_t _min_xc; 		// Minimum x coordinate
	int16_t _min_yc; 		// Minimum y coordinate
	int16_t _max_xc; 		// Maximum x coordinate
	int16_t _max_yc; 		// Maximum y coordinate
} TFT_t;

// para inicializar el display:
void lcdInitDisplay();							// solo para display ILI9341
void lcdInitFonts(int size, ...);				// si se usan fonts
void lcdClearScreen(uint16_t color);
void lcdDisplayOff();
void lcdDisplayOn();
void lcdInversionOff();
void lcdInversionOn();

void lcdDrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void lcdDrawRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void lcdDrawRectAngle(uint16_t xc, uint16_t yc, uint16_t w, uint16_t h, uint16_t angle, uint16_t color);
void lcdDrawTriangle(uint16_t xc, uint16_t yc, uint16_t w, uint16_t h, uint16_t angle, uint16_t color);
void lcdDrawCircle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);
void lcdDrawFillCircle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);
void lcdDrawRoundRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t r, uint16_t color);
void lcdDrawArrow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t w, uint16_t color);
void lcdDrawFillArrow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t w, uint16_t color);
void lcdDrawPixel(uint16_t x, uint16_t y, uint16_t color);
void lcdDrawMultiPixels(uint16_t x, uint16_t y, uint16_t size, uint16_t *colors);
void lcdDrawFillRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

int lcdPrintChar(uint16_t x, uint16_t y, char ascii);
int lcdPrintAtPos(uint16_t x, uint16_t y, char *texto);
int lcdPrintf(const char *format, ...); // imprime siempre donde dejo el anterior printf
void lcdSetCursor(uint16_t x, uint16_t y); // lo usa el lcdPrintf

void lcdGetFont(FontxFile *fx);	// retorna el font seleccionado
void lcdSetFontEx(int font_index, FontxFile *fx);
void lcdSetFont(int fontIndex); // cambia el font al font index cargado con InitFont
void lcdSetAndGetFont(int font_index, FontxFile *fx); // retorna un font_caps
void lcdSetFontColor(uint16_t color);
void lcdSetFontDirection(uint16_t);
void lcdSetFontFill(uint16_t color);
void lcdUnsetFontFill();
void lcdSetFontUnderLine(uint16_t color);
void lcdUnsetFontUnderLine();

void lcdBacklightOff();
void lcdBacklightOn();

void lcdSetScrollArea(uint16_t tfa, uint16_t vsa, uint16_t bfa);
void lcdResetScrollArea(uint16_t vsa);
void lcdScroll(uint16_t vsp);

// carga imagenes desde el FS y las muestra en el display:
void lcdLoadJpg(int x, int y, const char *fileName);
void lcdLoadPng(int x, int y, const char *fileName);
void lcdLoadBmp(int x, int y, const char *fileName);

int xptGetit(int cmd);
void xptGetxy(int *xp, int *yp);

#endif /* MAIN_ILI9340_H_ */
