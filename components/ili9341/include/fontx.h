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
	char *path;			// guardado el nombre del archivo (real del FS) cuando se inicializaron las fonts
	bool opened; 		// para no volver a abrir el archivo
	uint8_t width; 		// ancho y alto de la font
	uint8_t height;
	uint16_t fsz;		// tamaño de cada letra
	FILE *file; 		// archivo abierto (queda abierto!)
	uint8_t *bmpFont; 	// buffer con el bitmap del font seleccionado (lo usa GetFontx)
} FontxFile;

void InitFontx(FontxFile *fx, char *root, char *fontName);
bool OpenFontx(FontxFile *fx);
void CloseFontx(FontxFile *fx);
FontxFile* GetFontx(FontxFile *fxs, char ascii, uint8_t *pw, uint8_t *ph);
void Font2Bitmap(uint8_t *fonts, uint8_t *line, uint8_t w, uint8_t h, uint8_t inverse);
void UnderlineBitmap(uint8_t *line, uint8_t w, uint8_t h);
void ReversBitmap(uint8_t *line, uint8_t w, uint8_t h);
void ShowFont(uint8_t *fonts, uint8_t pw, uint8_t ph);
void ShowBitmap(uint8_t *bitmap, uint8_t pw, uint8_t ph);
uint8_t RotateByte(uint8_t ch);

#endif /* MAIN_FONTX_H_ */

