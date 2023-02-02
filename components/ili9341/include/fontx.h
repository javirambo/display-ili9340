#ifndef MAIN_FONTX_H_
#define MAIN_FONTX_H_

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <stdbool.h>

#define FontxGlyphBufSize (32*32/8)

typedef struct
{
	char *name;			// guardado el nombre del archivo (por si alguien quiere saber cual es?)
	uint8_t width; 		// ancho y alto de la font
	uint8_t height;
	uint16_t fsz;		// tamaño de cada letra
	FILE *file; 		// archivo abierto (queda abierto!)
	uint8_t *bmpFont; 	// buffer con el bitmap del font seleccionado (lo usa GetFontx)
} FontxFile;

void InitFont(FontxFile *fx, char *fontName);
void GetFont(FontxFile *fxs, char ascii, uint8_t *pw, uint8_t *ph);

// solo para debug
void ShowFont(uint8_t *fonts, uint8_t pw, uint8_t ph);
void ShowBitmap(uint8_t *bitmap, uint8_t pw, uint8_t ph);

// no se para que son
void Font2Bitmap(uint8_t *fonts, uint8_t *line, uint8_t w, uint8_t h, uint8_t inverse);
void UnderlineBitmap(uint8_t *line, uint8_t w, uint8_t h);
void ReverseBitmap(uint8_t *line, uint8_t w, uint8_t h);

#endif /* MAIN_FONTX_H_ */

