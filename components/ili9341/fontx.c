#include "esp_err.h"
#include "esp_log.h"

#include "fontx.h"

static const char *TAG = "fontx";

void InitFontx(FontxFile *fx, char *root, char *fontName)
{
	memset(fx, 0, sizeof(FontxFile));
	fx->path = (char*) malloc(strlen(root) + strlen(fontName) + 2);
	strcpy(fx->path, root);
	strcat(fx->path, "/");
	strcat(fx->path, fontName);
	fx->opened = false;
	fx->bmpFont = NULL; // se inicializa con GetFontx
}

bool OpenFontx(FontxFile *fx)
{
	if (fx->opened)
		return true;
	FILE *f = fopen(fx->path, "r");
	if (f)
	{
		char buf[18];
		fread(buf, 1, sizeof(buf), f);
		fx->width = buf[14];
		fx->height = buf[15];
		fx->fsz = (fx->width + 7) / 8 * fx->height;
		fx->opened = true;
		fx->file = f;
		return (fx->fsz <= FontxGlyphBufSize);
	}
	return false;
}

void CloseFontx(FontxFile *fx)
{
	if (fx->opened)
	{
		fclose(fx->file);
		fx->opened = false;
		free(fx->path); // chau buffer del nombre de la font.
	}
}

/**
 * Extraer patrones de fuentes de archivos de fuentes

 Disposición de fuente (16X16 puntos)
 00000000	01111111
 12345678	90123456
 01 pGlyph[000] pGlyph[001]
 02 pGlyph[002] pGlyph[003]
 03 pGlyph[004] pGlyph[005]
 04 pGlyph[006] pGlyph[007]
 05 pGlyph[008] pGlyph[009]
 06 pGlyph[010] pGlyph[011]
 07 pGlyph[012] pGlyph[013]
 08 pGlyph[014] pGlyph[015]
 09 pGlyph[016] pGlyph[017]
 10 pGlyph[018] pGlyph[019]
 11 pGlyph[020] pGlyph[021]
 12 pGlyph[022] pGlyph[023]
 13 pGlyph[024] pGlyph[025]
 14 pGlyph[026] pGlyph[027]
 15 pGlyph[028] pGlyph[029]
 16 pGlyph[030] pGlyph[031]

 Disposición de fuentes (24X24 puntos)
 00000000	01111111	11122222
 12345678	90123456	78901234
 01 pGlyph[000] pGlyph[001] pGlyph[002]
 02 pGlyph[003] pGlyph[004] pGlyph[005]
 03 pGlyph[006] pGlyph[007] pGlyph[008]
 04 pGlyph[009] pGlyph[010] pGlyph[011]
 05 pGlyph[012] pGlyph[013] pGlyph[014]
 06 pGlyph[015] pGlyph[016] pGlyph[017]
 07 pGlyph[018] pGlyph[019] pGlyph[020]
 08 pGlyph[021] pGlyph[022] pGlyph[023]
 09 pGlyph[024] pGlyph[025] pGlyph[026]
 10 pGlyph[027] pGlyph[028] pGlyph[029]
 11 pGlyph[030] pGlyph[031] pGlyph[032]
 12 pGlyph[033] pGlyph[034] pGlyph[035]
 13 pGlyph[036] pGlyph[037] pGlyph[038]
 14 pGlyph[039] pGlyph[040] pGlyph[041]
 15 pGlyph[042] pGlyph[043] pGlyph[044]
 16 pGlyph[045] pGlyph[046] pGlyph[047]
 17 pGlyph[048] pGlyph[049] pGlyph[050]
 18 pGlyph[051] pGlyph[052] pGlyph[053]
 19 pGlyph[054] pGlyph[055] pGlyph[056]
 20 pGlyph[057] pGlyph[058] pGlyph[059]
 21 pGlyph[060] pGlyph[061] pGlyph[062]
 22 pGlyph[063] pGlyph[064] pGlyph[065]
 23 pGlyph[066] pGlyph[067] pGlyph[068]
 24 pGlyph[069] pGlyph[070] pGlyph[071]

 Disposición de fuente (32X32 puntos)
 00000000	01111111	11122222	22222333
 12345678	90123456	78901234	56789012
 01 pGlyph[000] pGlyph[001] pGlyph[002] pGlyph[003]
 02 pGlyph[004] pGlyph[005] pGlyph[006] pGlyph[007]
 03 pGlyph[008] pGlyph[009] pGlyph[010] pGlyph[011]
 04 pGlyph[012] pGlyph[013] pGlyph[014] pGlyph[015]
 05 pGlyph[016] pGlyph[017] pGlyph[018] pGlyph[019]
 06 pGlyph[020] pGlyph[021] pGlyph[022] pGlyph[023]
 07 pGlyph[024] pGlyph[025] pGlyph[026] pGlyph[027]
 08 pGlyph[028] pGlyph[029] pGlyph[030] pGlyph[031]
 09 pGlyph[032] pGlyph[033] pGlyph[034] pGlyph[035]
 10 pGlyph[036] pGlyph[037] pGlyph[038] pGlyph[039]
 11 pGlyph[040] pGlyph[041] pGlyph[042] pGlyph[043]
 12 pGlyph[044] pGlyph[045] pGlyph[046] pGlyph[047]
 13 pGlyph[048] pGlyph[049] pGlyph[050] pGlyph[051]
 14 pGlyph[052] pGlyph[053] pGlyph[054] pGlyph[055]
 15 pGlyph[056] pGlyph[057] pGlyph[058] pGlyph[059]
 16 pGlyph[060] pGlyph[061] pGlyph[062] pGlyph[063]
 17 pGlyph[064] pGlyph[065] pGlyph[066] pGlyph[067]
 18 pGlyph[068] pGlyph[069] pGlyph[070] pGlyph[071]
 19 pGlyph[072] pGlyph[073] pGlyph[074] pGlyph[075]
 20 pGlyph[076] pGlyph[077] pGlyph[078] pGlyph[079]
 21 pGlyph[080] pGlyph[081] pGlyph[082] pGlyph[083]
 22 pGlyph[084] pGlyph[085] pGlyph[086] pGlyph[087]
 23 pGlyph[088] pGlyph[089] pGlyph[090] pGlyph[091]
 24 pGlyph[092] pGlyph[093] pGlyph[094] pGlyph[095]
 25 pGlyph[096] pGlyph[097] pGlyph[098] pGlyph[099]
 26 pGlyph[100] pGlyph[101] pGlyph[102] pGlyph[103]
 27 pGlyph[104] pGlyph[105] pGlyph[106] pGlyph[107]
 28 pGlyph[108] pGlyph[109] pGlyph[110] pGlyph[111]
 29 pGlyph[112] pGlyph[113] pGlyph[114] pGlyph[115]
 30 pGlyph[116] pGlyph[117] pGlyph[118] pGlyph[119]
 31 pGlyph[120] pGlyph[121] pGlyph[122] pGlyph[123]
 32 pGlyph[124] pGlyph[125] pGlyph[127] pGlyph[128]
 */

FontxFile* GetFontx(FontxFile *fx, char ascii, uint8_t *pw, uint8_t *ph)
{
	if (OpenFontx(fx))
	{
		if (fx->bmpFont == NULL)
			fx->bmpFont = (uint8_t*) malloc(fx->fsz);

		uint32_t offset = 17 + (((uint8_t) ascii) * fx->fsz);
		fseek(fx->file, offset, SEEK_SET);
		fread(fx->bmpFont, 1, fx->fsz, fx->file);
		if (pw)
			*pw = fx->width;
		if (ph)
			*ph = fx->height;
	}
	return fx;
}

/*
 Convierta patrones de fuente en imágenes de mapa de bits

 fonts(16X16 puntos)
 00000000	01111111
 12345678	90123456
 01 pGlyph[000] pGlyph[001]
 02 pGlyph[002] pGlyph[003]
 03 pGlyph[004] pGlyph[005]
 04 pGlyph[006] pGlyph[007]
 05 pGlyph[008] pGlyph[009]
 06 pGlyph[010] pGlyph[011]
 07 pGlyph[012] pGlyph[013]
 08 pGlyph[014] pGlyph[015]
 09 pGlyph[016] pGlyph[017]
 10 pGlyph[018] pGlyph[019]
 11 pGlyph[020] pGlyph[021]
 12 pGlyph[022] pGlyph[023]
 13 pGlyph[024] pGlyph[025]
 14 pGlyph[026] pGlyph[027]
 15 pGlyph[028] pGlyph[029]
 16 pGlyph[030] pGlyph[031]

 line[32*4]
 01 line[000] line[001] line[002] .... line[014] line[015] line[016-031](Not use)
 |
 07 line[000] line[001] line[002] .... line[014] line[015] line[016-031](Not use)

 08 line[032] line[033] line[034] .... line[046] line[047] line[048-063](Not use)
 |
 16 line[032] line[033] line[034] .... line[046] line[047] line[048-063](Not use)



 fonts(24X24 puntos)
 00000000	01111111	11122222
 12345678	90123456	78901234
 01 pGlyph[000] pGlyph[001] pGlyph[002]
 02 pGlyph[003] pGlyph[004] pGlyph[005]
 03 pGlyph[006] pGlyph[007] pGlyph[008]
 04 pGlyph[009] pGlyph[010] pGlyph[011]
 05 pGlyph[012] pGlyph[013] pGlyph[014]
 06 pGlyph[015] pGlyph[016] pGlyph[017]
 07 pGlyph[018] pGlyph[019] pGlyph[020]
 08 pGlyph[021] pGlyph[022] pGlyph[023]
 09 pGlyph[024] pGlyph[025] pGlyph[026]
 10 pGlyph[027] pGlyph[028] pGlyph[029]
 11 pGlyph[030] pGlyph[031] pGlyph[032]
 12 pGlyph[033] pGlyph[034] pGlyph[035]
 13 pGlyph[036] pGlyph[037] pGlyph[038]
 14 pGlyph[039] pGlyph[040] pGlyph[041]
 15 pGlyph[042] pGlyph[043] pGlyph[044]
 16 pGlyph[045] pGlyph[046] pGlyph[047]
 17 pGlyph[048] pGlyph[049] pGlyph[050]
 18 pGlyph[051] pGlyph[052] pGlyph[053]
 19 pGlyph[054] pGlyph[055] pGlyph[056]
 20 pGlyph[057] pGlyph[058] pGlyph[059]
 21 pGlyph[060] pGlyph[061] pGlyph[062]
 22 pGlyph[063] pGlyph[064] pGlyph[065]
 23 pGlyph[066] pGlyph[067] pGlyph[068]
 24 pGlyph[069] pGlyph[070] pGlyph[071]

 line[32*4]
 01 line[000] line[001] line[002] .... line[022] line[023] line[024-031](Not use)
 |
 08 line[000] line[001] line[002] .... line[022] line[023] line[024-031](Not use)

 09 line[032] line[033] line[034] .... line[054] line[055] line[056-063](Not use)
 |
 16 line[032] line[033] line[034] .... line[054] line[055] line[056-063](Not use)

 17 line[064] line[065] line[066] .... line[086] line[087] line[088-095](Not use)
 |
 24 line[064] line[065] line[066] .... line[086] line[087] line[088-095](Not use)


 fonts(32X32 puntos)
 00000000	01111111	11122222	22222333
 12345678	90123456	78901234	56789012
 01 pGlyph[000] pGlyph[001] pGlyph[002] pGlyph[003]
 02 pGlyph[004] pGlyph[005] pGlyph[006] pGlyph[007]
 03 pGlyph[008] pGlyph[009] pGlyph[010] pGlyph[011]
 04 pGlyph[012] pGlyph[013] pGlyph[014] pGlyph[015]
 05 pGlyph[016] pGlyph[017] pGlyph[018] pGlyph[019]
 06 pGlyph[020] pGlyph[021] pGlyph[022] pGlyph[023]
 07 pGlyph[024] pGlyph[025] pGlyph[026] pGlyph[027]
 08 pGlyph[028] pGlyph[029] pGlyph[030] pGlyph[031]
 09 pGlyph[032] pGlyph[033] pGlyph[034] pGlyph[035]
 10 pGlyph[036] pGlyph[037] pGlyph[038] pGlyph[039]
 11 pGlyph[040] pGlyph[041] pGlyph[042] pGlyph[043]
 12 pGlyph[044] pGlyph[045] pGlyph[046] pGlyph[047]
 13 pGlyph[048] pGlyph[049] pGlyph[050] pGlyph[051]
 14 pGlyph[052] pGlyph[053] pGlyph[054] pGlyph[055]
 15 pGlyph[056] pGlyph[057] pGlyph[058] pGlyph[059]
 16 pGlyph[060] pGlyph[061] pGlyph[062] pGlyph[063]
 17 pGlyph[064] pGlyph[065] pGlyph[066] pGlyph[067]
 18 pGlyph[068] pGlyph[069] pGlyph[070] pGlyph[071]
 19 pGlyph[072] pGlyph[073] pGlyph[074] pGlyph[075]
 20 pGlyph[076] pGlyph[077] pGlyph[078] pGlyph[079]
 21 pGlyph[080] pGlyph[081] pGlyph[082] pGlyph[083]
 22 pGlyph[084] pGlyph[085] pGlyph[086] pGlyph[087]
 23 pGlyph[088] pGlyph[089] pGlyph[090] pGlyph[091]
 24 pGlyph[092] pGlyph[093] pGlyph[094] pGlyph[095]
 25 pGlyph[096] pGlyph[097] pGlyph[098] pGlyph[099]
 26 pGlyph[100] pGlyph[101] pGlyph[102] pGlyph[103]
 27 pGlyph[104] pGlyph[105] pGlyph[106] pGlyph[107]
 28 pGlyph[108] pGlyph[109] pGlyph[110] pGlyph[111]
 29 pGlyph[112] pGlyph[113] pGlyph[114] pGlyph[115]
 30 pGlyph[116] pGlyph[117] pGlyph[118] pGlyph[119]
 31 pGlyph[120] pGlyph[121] pGlyph[122] pGlyph[123]
 32 pGlyph[124] pGlyph[125] pGlyph[127] pGlyph[128]

 line[32*4]
 01 line[000] line[001] line[002] .... line[030] line[031]
 |
 08 line[000] line[001] line[002] .... line[030] line[031]

 09 line[032] line[033] line[034] .... line[062] line[063]
 |
 16 line[032] line[033] line[034] .... line[062] line[063]

 17 line[064] line[065] line[066] .... line[094] line[095]
 |
 24 line[064] line[065] line[066] .... line[094] line[095]

 25 line[096] line[097] line[098] .... line[126] line[127]
 |
 32 line[096] line[097] line[098] .... line[126] line[127]
 */

void Font2Bitmap(uint8_t *fonts, uint8_t *line, uint8_t w, uint8_t h, uint8_t inverse)
{
	int x, y;
	for (y = 0; y < (h / 8); y++)
	{
		for (x = 0; x < w; x++)
		{
			line[y * 32 + x] = 0;
		}
	}

	int mask = 7;
	int fontp;
	fontp = 0;
	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w; x++)
		{
			uint8_t d = fonts[fontp + x / 8];
			uint8_t linep = (y / 8) * 32 + x;
			if (d & (0x80 >> (x % 8)))
				line[linep] = line[linep] + (1 << mask);
		}
		mask--;
		if (mask < 0)
			mask = 7;
		fontp += (w + 7) / 8;
	}

	if (inverse)
	{
		for (y = 0; y < (h / 8); y++)
		{
			for (x = 0; x < w; x++)
			{
				line[y * 32 + x] = RotateByte(line[y * 32 + x]);
			}
		}
	}
}

// Agrega subrayado
void UnderlineBitmap(uint8_t *line, uint8_t w, uint8_t h)
{
	int x, y;
	uint8_t wk;
	for (y = 0; y < (h / 8); y++)
	{
		for (x = 0; x < w; x++)
		{
			wk = line[y * 32 + x];
			if ((y + 1) == (h / 8))
				line[y * 32 + x] = wk + 0x80;
		}
	}
}

// Invierte mapa de bits
void ReversBitmap(uint8_t *line, uint8_t w, uint8_t h)
{
	int x, y;
	uint8_t wk;
	for (y = 0; y < (h / 8); y++)
	{
		for (x = 0; x < w; x++)
		{
			wk = line[y * 32 + x];
			line[y * 32 + x] = ~wk;
		}
	}
}

// Mostrar patrones de fuente
void ShowFont(uint8_t *fonts, uint8_t pw, uint8_t ph)
{
	int x, y, fpos;
	printf("[ShowFont pw=%d ph=%d]\n", pw, ph);
	fpos = 0;
	for (y = 0; y < ph; y++)
	{
		printf("%02d", y);
		for (x = 0; x < pw; x++)
		{
			if (fonts[fpos + x / 8] & (0x80 >> (x % 8)))
			{
				printf("*");
			}
			else
			{
				printf(".");
			}
		}
		printf("\n");
		fpos = fpos + (pw + 7) / 8;
	}
	printf("\n");
}

void ShowBitmap(uint8_t *bitmap, uint8_t pw, uint8_t ph)
{
	int x, y, fpos;
	printf("[ShowBitmap pw=%d ph=%d]\n", pw, ph);
#if 0
	for (y=0;y<(ph+7)/8;y++) {
		for (x=0;x<pw;x++) {
			printf("%02x ",bitmap[x+y*32]);
		}
		printf("\n");
	}
#endif

	fpos = 0;
	for (y = 0; y < ph; y++)
	{
		printf("%02d", y);
		for (x = 0; x < pw; x++)
		{
//printf("b=%x m=%x\n",bitmap[x+(y/8)*32],0x80 >> fpos);
			if (bitmap[x + (y / 8) * 32] & (0x80 >> fpos))
			{
				printf("*");
			}
			else
			{
				printf(".");
			}
		}
		printf("\n");
		fpos++;
		if (fpos > 7)
			fpos = 0;
	}
	printf("\n");
}

// invertir datos
uint8_t RotateByte(uint8_t ch1)
{
	uint8_t ch2 = 0;
	int j;
	for (j = 0; j < 8; j++)
	{
		ch2 = (ch2 << 1) + (ch1 & 0x01);
		ch1 = ch1 >> 1;
	}
	return ch2;
}

