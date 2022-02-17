/*
 * bmp_image.c
 *
 *  Created on: 16 feb. 2022
 *      Author: Javier
 */

#include "esp_log.h"
#include "bmpfile.h"

#define BUFFPIXEL 20

int load_bmp(TFT_t *dev, int _x, int _y, char *file, int scr_width, int scr_height)
{
	lcdSetFontDirection(dev, 0);
	lcdFillScreen(dev, BLACK);

	// open BMP file
	esp_err_t ret;
	FILE *fp = fopen(file, "rb");
	if (fp == NULL)
	{
		ESP_LOGW(__FUNCTION__, "File not found [%s]", file);
		return 0; // error
	}

	// read bmp header
	bmpfile_t *result = (bmpfile_t*) malloc(sizeof(bmpfile_t));
	ret = fread(result->header.magic, 1, 2, fp);
	assert(ret == 2);
	ESP_LOGD(__FUNCTION__, "result->header.magic=%c %c", result->header.magic[0], result->header.magic[1]);
	if (result->header.magic[0] != 'B' || result->header.magic[1] != 'M')
	{
		ESP_LOGW(__FUNCTION__, "File is not BMP");
		free(result);
		fclose(fp);
		return 0; // error
	}
	ret = fread(&result->header.filesz, 4, 1, fp);
	assert(ret == 1);
	ESP_LOGD(__FUNCTION__, "result->header.filesz=%d", result->header.filesz);
	ret = fread(&result->header.creator1, 2, 1, fp);
	assert(ret == 1);
	ret = fread(&result->header.creator2, 2, 1, fp);
	assert(ret == 1);
	ret = fread(&result->header.offset, 4, 1, fp);
	assert(ret == 1);

	// read dib header
	ret = fread(&result->dib.header_sz, 4, 1, fp);
	assert(ret == 1);
	ret = fread(&result->dib.width, 4, 1, fp);
	assert(ret == 1);
	ret = fread(&result->dib.height, 4, 1, fp);
	assert(ret == 1);
	ret = fread(&result->dib.nplanes, 2, 1, fp);
	assert(ret == 1);
	ret = fread(&result->dib.depth, 2, 1, fp);
	assert(ret == 1);
	ret = fread(&result->dib.compress_type, 4, 1, fp);
	assert(ret == 1);
	ret = fread(&result->dib.bmp_bytesz, 4, 1, fp);
	assert(ret == 1);
	ret = fread(&result->dib.hres, 4, 1, fp);
	assert(ret == 1);
	ret = fread(&result->dib.vres, 4, 1, fp);
	assert(ret == 1);
	ret = fread(&result->dib.ncolors, 4, 1, fp);
	assert(ret == 1);
	ret = fread(&result->dib.nimpcolors, 4, 1, fp);
	assert(ret == 1);

	if ((result->dib.depth == 24) && (result->dib.compress_type == 0))
	{
		// BMP rows are padded (if needed) to 4-byte boundary
		uint32_t rowSize = (result->dib.width * 3 + 3) & ~3;
		int w = result->dib.width;
		int h = result->dib.height;
		ESP_LOGD(__FUNCTION__, "w=%d h=%d", w, h);
		//int _x;
		int _w;
		int _cols;
		int _cole;
		if (scr_width >= w)
		{
			_x = (scr_width - w) / 2;
			_w = w;
			_cols = 0;
			_cole = w - 1;
		}
		else
		{
			_x = 0;
			_w = scr_width;
			_cols = (w - scr_width) / 2;
			_cole = _cols + scr_width - 1;
		}
		ESP_LOGD(__FUNCTION__, "_x=%d _w=%d _cols=%d _cole=%d", _x, _w, _cols, _cole);

		//int _y;
		int _rows;
		int _rowe;
		if (scr_height >= h)
		{
			_y = (scr_height - h) / 2;
			_rows = 0;
			_rowe = h - 1;
		}
		else
		{
			_y = 0;
			_rows = (h - scr_height) / 2;
			_rowe = _rows + scr_height - 1;
		}
		ESP_LOGD(__FUNCTION__, "_y=%d _rows=%d _rowe=%d", _y, _rows, _rowe);

		uint8_t sdbuffer[3 * BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
		uint16_t *colors = (uint16_t*) malloc(sizeof(uint16_t) * w);

		for (int row = 0; row < h; row++)
		{ // For each scanline...
			if (row < _rows || row > _rowe)
				continue;
			// Seek to start of scan line.	It might seem labor-
			// intensive to be doing this on every line, but this
			// method covers a lot of gritty details like cropping
			// and scanline padding.	Also, the seek only takes
			// place if the file position actually needs to change
			// (avoids a lot of cluster math in SD library).
			// Bitmap is stored bottom-to-top order (normal BMP)
			int pos = result->header.offset + (h - 1 - row) * rowSize;
			fseek(fp, pos, SEEK_SET);
			int buffidx = sizeof(sdbuffer); // Force buffer reload

			int index = 0;
			for (int col = 0; col < w; col++)
			{ // For each pixel...
				if (buffidx >= sizeof(sdbuffer))
				{ // Indeed
					fread(sdbuffer, sizeof(sdbuffer), 1, fp);
					buffidx = 0; // Set index to beginning
				}
				if (col < _cols || col > _cole)
					continue;
				// Convert pixel from BMP to TFT format, push to display
				uint8_t b = sdbuffer[buffidx++];
				uint8_t g = sdbuffer[buffidx++];
				uint8_t r = sdbuffer[buffidx++];
				colors[index++] = rgb565_conv(r, g, b);
			} // end for col
			ESP_LOGD(__FUNCTION__, "lcdDrawMultiPixels row=%d", row);
			//lcdDrawMultiPixels(dev, _x, row+_y, _w, colors);
			lcdDrawMultiPixels(dev, _x, _y, _w, colors);
			_y++;
		} // end for row
		free(colors);
	} // end if
	free(result);
	fclose(fp);
	return 1;
}
