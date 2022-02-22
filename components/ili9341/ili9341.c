/*
 * https://github.com/nopnop2002/esp-idf-ili9340
 */
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <ili9341.h>
#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"

#include "fontx.h"
#include "bmpfile.h"
#include "decode_jpeg.h"
#include "decode_png.h"
#include "ili9341.h"
#include "pngle.h"

#define TAM_PRINT_BUF 512

static const char *TAG = "ILI9341";

static TFT_t dev;					// handle al display
static FontxFile *fonts; 			// guarda todas las fonts a usar
static char *fs_root_name = NULL;	// maneja el FS (si se usan fonts se usa el FS)

#ifdef CONFIG_IDF_TARGET_ESP32
#define LCD_HOST HSPI_HOST
#elif defined CONFIG_IDF_TARGET_ESP32S2
#define LCD_HOST SPI2_HOST
#elif defined CONFIG_IDF_TARGET_ESP32C3
#define LCD_HOST SPI2_HOST
#endif

static const int SPI_Command_Mode = 0;
static const int SPI_Data_Mode = 1;
static const int TFT_Frequency = SPI_MASTER_FREQ_40M;

static void spi_master_init()
{
	esp_err_t ret;

	ESP_LOGI(TAG, "TFT_CS=%d", CONFIG_TFT_CS_GPIO);
	//gpio_pad_select_gpio( CONFIG_TFT_CS_GPIO );
	gpio_reset_pin(CONFIG_TFT_CS_GPIO);
	gpio_set_direction(CONFIG_TFT_CS_GPIO, GPIO_MODE_OUTPUT);
	//gpio_set_level( CONFIG_TFT_CS_GPIO, 0 );
	gpio_set_level(CONFIG_TFT_CS_GPIO, 1);

	ESP_LOGI(TAG, "TFT_DC=%d", CONFIG_DC_GPIO);
	//gpio_pad_select_gpio( CONFIG_DC_GPIO );
	gpio_reset_pin(CONFIG_DC_GPIO);
	gpio_set_direction(CONFIG_DC_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_level(CONFIG_DC_GPIO, 0);

	ESP_LOGI(TAG, "GPIO_RESET=%d", CONFIG_RESET_GPIO);
	if (CONFIG_RESET_GPIO >= 0)
	{
		//gpio_pad_select_gpio( GPIO_RESET );
		gpio_reset_pin(CONFIG_RESET_GPIO);
		gpio_set_direction(CONFIG_RESET_GPIO, GPIO_MODE_OUTPUT);
		gpio_set_level(CONFIG_RESET_GPIO, 0);
		vTaskDelay(pdMS_TO_TICKS(100));
		gpio_set_level(CONFIG_RESET_GPIO, 1);
	}

	ESP_LOGI(TAG, "GPIO_BL=%d", CONFIG_BL_GPIO);
	if (CONFIG_BL_GPIO >= 0)
	{
		//gpio_pad_select_gpio( CONFIG_BL_GPIO );
		gpio_reset_pin(CONFIG_BL_GPIO);
		gpio_set_direction(CONFIG_BL_GPIO, GPIO_MODE_OUTPUT);
		gpio_set_level(CONFIG_BL_GPIO, 0);
	}

#if CONFIG_XPT2046
	spi_bus_config_t buscfg = {
		.sclk_io_num = CONFIG_SCLK_GPIO,
		.mosi_io_num = CONFIG_MOSI_GPIO,
		.miso_io_num = CONFIG_MISO_GPIO,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1
	};
#else
	spi_bus_config_t buscfg = {
		.sclk_io_num = CONFIG_SCLK_GPIO,
		.mosi_io_num = CONFIG_MOSI_GPIO,
		.miso_io_num = CONFIG_MISO_GPIO, // -1 si no se usan comandos de READ
		.quadwp_io_num = -1,
		.quadhd_io_num = -1
	};
#endif

	ret = spi_bus_initialize( LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
	ESP_LOGD(TAG, "spi_bus_initialize=%d", ret);
	assert(ret==ESP_OK);

	spi_device_interface_config_t tft_devcfg = {
		.clock_speed_hz = TFT_Frequency,
		.spics_io_num = CONFIG_TFT_CS_GPIO,
		.queue_size = 7,
		.flags = SPI_DEVICE_NO_DUMMY,
	};

	spi_device_handle_t tft_handle;
	ret = spi_bus_add_device( LCD_HOST, &tft_devcfg, &tft_handle);
	ESP_LOGD(TAG, "spi_bus_add_device=%d", ret);
	assert(ret==ESP_OK);
	dev._dc = CONFIG_DC_GPIO;
	dev._bl = CONFIG_BL_GPIO;
	dev._TFT_Handle = tft_handle;

#if CONFIG_XPT2046
	ESP_LOGI(TAG, "XPT_CS=%d",CONFIG_XPT_CS_GPIO);
	//gpio_pad_select_gpio( CONFIG_XPT_CS_GPIO );
	gpio_reset_pin( CONFIG_XPT_CS_GPIO );
	gpio_set_direction( CONFIG_XPT_CS_GPIO, GPIO_MODE_OUTPUT );
	gpio_set_level( CONFIG_XPT_CS_GPIO, 1 );

	// set the IRQ as a input
	ESP_LOGI(TAG, "XPT_IRQ=%d",CONFIG_XPT_IRQ_GPIO);
	gpio_config_t io_conf = {};
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.pin_bit_mask = (1ULL<< CONFIG_XPT_IRQ_GPIO);
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_up_en = 1;
	gpio_config(&io_conf);
	//gpio_reset_pin( CONFIG_XPT_IRQ_GPIO );
	//gpio_set_direction( CONFIG_XPT_IRQ_GPIO, GPIO_MODE_DEF_INPUT );

	spi_device_interface_config_t xpt_devcfg={
		.clock_speed_hz = XPT_Frequency,
		.spics_io_num = CONFIG_XPT_CS_GPIO,
		.queue_size = 7,
		.flags = SPI_DEVICE_NO_DUMMY,
	};

	spi_device_handle_t xpt_handle;
	ret = spi_bus_add_device( LCD_HOST, &xpt_devcfg, &xpt_handle);
	ESP_LOGD(TAG, "spi_bus_add_device=%d",ret);
	assert(ret==ESP_OK);
	dev._XPT_Handle = xpt_handle;
	dev._irq = CONFIG_XPT_IRQ_GPIO;
	dev._calibration = true;
#endif
}

static bool spi_master_write_byte(spi_device_handle_t SPIHandle, const uint8_t *Data, size_t DataLength)
{
	spi_transaction_t SPITransaction;
	esp_err_t ret;

	if (DataLength > 0)
	{
		memset(&SPITransaction, 0, sizeof(spi_transaction_t));
		SPITransaction.length = DataLength * 8;
		SPITransaction.tx_buffer = Data;
#if 1
		ret = spi_device_transmit(SPIHandle, &SPITransaction);
#endif
#if 0
		ret = spi_device_polling_transmit( SPIHandle, &SPITransaction );
#endif
		assert(ret==ESP_OK);
	}

	return true;
}

static bool spi_master_write_comm_byte(uint8_t cmd)
{
	static uint8_t Byte = 0;
	Byte = cmd;
	gpio_set_level(dev._dc, SPI_Command_Mode);
	return spi_master_write_byte(dev._TFT_Handle, &Byte, 1);
}

#if 0
static bool spi_master_write_comm_word(uint16_t cmd)
{
	static uint8_t Byte[2];
	Byte[0] = (cmd >> 8) & 0xFF;
	Byte[1] = cmd & 0xFF;
	gpio_set_level(dev._dc, SPI_Command_Mode);
	return spi_master_write_byte(dev._TFT_Handle, Byte, 2);
}
#endif

static bool spi_master_write_data_byte(uint8_t data)
{
	static uint8_t Byte = 0;
	Byte = data;
	gpio_set_level(dev._dc, SPI_Data_Mode);
	return spi_master_write_byte(dev._TFT_Handle, &Byte, 1);
}

static bool spi_master_write_data_word(uint16_t data)
{
	static uint8_t Byte[2];
	Byte[0] = (data >> 8) & 0xFF;
	Byte[1] = data & 0xFF;
	gpio_set_level(dev._dc, SPI_Data_Mode);
	return spi_master_write_byte(dev._TFT_Handle, Byte, 2);
}

static bool spi_master_write_addr(uint16_t addr1, uint16_t addr2)
{
	static uint8_t Byte[4];
	Byte[0] = (addr1 >> 8) & 0xFF;
	Byte[1] = addr1 & 0xFF;
	Byte[2] = (addr2 >> 8) & 0xFF;
	Byte[3] = addr2 & 0xFF;
	gpio_set_level(dev._dc, SPI_Data_Mode);
	return spi_master_write_byte(dev._TFT_Handle, Byte, 4);
}

static bool spi_master_write_color(uint16_t color, uint16_t size)
{
	static uint8_t Byte[1024];
	int index = 0;
	for (int i = 0; i < size; i++)
	{
		Byte[index++] = (color >> 8) & 0xFF;
		Byte[index++] = color & 0xFF;
	}
	gpio_set_level(dev._dc, SPI_Data_Mode);
	return spi_master_write_byte(dev._TFT_Handle, Byte, size * 2);
}

// Add 202001
static bool spi_master_write_colors(uint16_t *colors, uint16_t size)
{
	static uint8_t Byte[1024];
	int index = 0;
	for (int i = 0; i < size; i++)
	{
		Byte[index++] = (colors[i] >> 8) & 0xFF;
		Byte[index++] = colors[i] & 0xFF;
	}
	gpio_set_level(dev._dc, SPI_Data_Mode);
	return spi_master_write_byte(dev._TFT_Handle, Byte, size * 2);
}

static void delayMS(int ms)
{
	int _ms = ms + (portTICK_PERIOD_MS - 1);
	TickType_t xTicksToDelay = _ms / portTICK_PERIOD_MS;
	vTaskDelay(xTicksToDelay);
}

#if 0
static void lcdWriteRegisterWord(uint16_t addr, uint16_t data)
{
	spi_master_write_comm_word( addr);
	spi_master_write_data_word( data);
}

static void lcdWriteRegisterByte(uint8_t addr, uint16_t data)
{
	spi_master_write_comm_byte( addr);
	spi_master_write_data_word( data);
}
#endif

static void spi_display_init()
{
	dev.width = CONFIG_WIDTH;
	dev.height = CONFIG_HEIGHT;
	dev.font.direction = DIRECTION0;
	dev.font.is_fill = false;
	dev.font.is_underline = false;

	ESP_LOGI(TAG, "Screen width:%d", dev.width);
	ESP_LOGI(TAG, "Screen height:%d", dev.height);
	spi_master_write_comm_byte(0xC0);	//Power Control 1
	spi_master_write_data_byte(0x23);

	spi_master_write_comm_byte(0xC1);	//Power Control 2
	spi_master_write_data_byte(0x10);

	spi_master_write_comm_byte(0xC5);	//VCOM Control 1
	spi_master_write_data_byte(0x3E);
	spi_master_write_data_byte(0x28);

	spi_master_write_comm_byte(0xC7);	//VCOM Control 2
	spi_master_write_data_byte(0x86);

	spi_master_write_comm_byte(0x36);	//Memory Access Control
	//0x36 Parameter:  [MY MX MV ML BGR MH 0 0]
#ifdef CONFIG_ROTATE_SCREEN
	spi_master_write_data_byte( 0x08);	//Right top start, BGR color filter panel
#else
	spi_master_write_data_byte(0b01111000); // asi se ve apaisado
#endif

	spi_master_write_comm_byte(0x3A);	//Pixel Format Set
	spi_master_write_data_byte(0x55);	//65K color: 16-bit/pixel

	spi_master_write_comm_byte(0x20);	//Display Inversion OFF

	spi_master_write_comm_byte(0xB1);	//Frame Rate Control
	spi_master_write_data_byte(0x00);
	spi_master_write_data_byte(0x18);

	spi_master_write_comm_byte(0xB6);	//Display Function Control
	spi_master_write_data_byte(0x08);
	spi_master_write_data_byte(0xA2);	// REV:1 GS:0 SS:0 SM:0
	spi_master_write_data_byte(0x27);
	spi_master_write_data_byte(0x00);

	spi_master_write_comm_byte(0x26);	//Gamma Set
	spi_master_write_data_byte(0x01);

	spi_master_write_comm_byte(0xE0);	//Positive Gamma Correction
	spi_master_write_data_byte(0x0F);
	spi_master_write_data_byte(0x31);
	spi_master_write_data_byte(0x2B);
	spi_master_write_data_byte(0x0C);
	spi_master_write_data_byte(0x0E);
	spi_master_write_data_byte(0x08);
	spi_master_write_data_byte(0x4E);
	spi_master_write_data_byte(0xF1);
	spi_master_write_data_byte(0x37);
	spi_master_write_data_byte(0x07);
	spi_master_write_data_byte(0x10);
	spi_master_write_data_byte(0x03);
	spi_master_write_data_byte(0x0E);
	spi_master_write_data_byte(0x09);
	spi_master_write_data_byte(0x00);

	spi_master_write_comm_byte(0xE1);	//Negative Gamma Correction
	spi_master_write_data_byte(0x00);
	spi_master_write_data_byte(0x0E);
	spi_master_write_data_byte(0x14);
	spi_master_write_data_byte(0x03);
	spi_master_write_data_byte(0x11);
	spi_master_write_data_byte(0x07);
	spi_master_write_data_byte(0x31);
	spi_master_write_data_byte(0xC1);
	spi_master_write_data_byte(0x48);
	spi_master_write_data_byte(0x08);
	spi_master_write_data_byte(0x0F);
	spi_master_write_data_byte(0x0C);
	spi_master_write_data_byte(0x31);
	spi_master_write_data_byte(0x36);
	spi_master_write_data_byte(0x0F);

	spi_master_write_comm_byte(0x11);	//Sleep Out
	delayMS(120);

	spi_master_write_comm_byte(0x29);	//Display ON

	if (dev._bl >= 0)
	{
		gpio_set_level(dev._bl, 1);
	}
}

/*
 * Inicializa las fonts.
 * Pasar la cantidad de fonts y el array de nombres (strings)
 * Estas fonts deben estar en el FS.
 */
void lcdInitFonts(int size, ...)
{
	if (fs_root_name == NULL)
	{
		ESP_LOGE(TAG, "Primero inicializar el FS!");
	}
	else
	{
		fonts = (FontxFile*) malloc(size * sizeof(FontxFile));
		int i;
		va_list ap;
		va_start(ap, size);
		for (i = 0; i < size; i++)
			InitFontx(&fonts[i], fs_root_name, (char*) va_arg(ap, char*));
		va_end(ap);
	}
}

/*
 * Inicializa el SPI y el display.
 */
void lcdInitDisplay()
{
	spi_master_init();  // inicializa el bus SPI
	spi_display_init(); // inicializa el display con comandos al display

#if CONFIG_INVERSION
	ESP_LOGI(TAG, "Enable Display Inversion");
	lcdInversionOn(&dev); // invierte colores
#endif

#if CONFIG_RGB_COLOR
	ESP_LOGI(TAG, "Change BGR filter to RGB filter");
	lcdBGRFilter(&dev);
#endif
}

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

/**
 * Inicializa el FS (si es que se usa) pero si se usan fonts hay que inicializarlo.
 */
void lcdInitFS(char *root_name, int max_files)
{
	ESP_LOGD(TAG, "Initializing SPIFFS");

	if (*root_name != '/')
	{
		// tiene que empezar con /
		fs_root_name = (char*) malloc(2 + strlen(root_name));
		strcpy(fs_root_name, "/");
		strcat(fs_root_name, root_name);
	}
	else
	{
		fs_root_name = (char*) malloc(1 + strlen(root_name));
		strcpy(fs_root_name, root_name);
	}

	esp_vfs_spiffs_conf_t conf = {
		.base_path = fs_root_name,
		.partition_label = NULL,
		.max_files = max_files,
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
		return; // CHAU! ERROR!
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

	//TODO: si falla agregar una / al final
	SPIFFS_Directory(fs_root_name);
}

// Draw pixel
// x:X coordinate
// y:Y coordinate
// color:color
void lcdDrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
	if (x >= dev.width)
		return;
	if (y >= dev.height)
		return;

	uint16_t _x = x;	// + dev._offsetx;
	uint16_t _y = y;	// + dev._offsety;

	spi_master_write_comm_byte(0x2A);	// set column(x) address
	spi_master_write_addr(_x, _x);
	spi_master_write_comm_byte(0x2B);	// set Page(y) address
	spi_master_write_addr(_y, _y);
	spi_master_write_comm_byte(0x2C);	// Memory Write
	spi_master_write_data_word(color);

}

// Add 202001
// Draw multi pixel
// x:X coordinate
// y:Y coordinate
// size:Number of colors
// colors:colors
void lcdDrawMultiPixels(uint16_t x, uint16_t y, uint16_t size, uint16_t *colors)
{
	if (x + size > dev.width)
		return;
	if (y >= dev.height)
		return;

	//ESP_LOGD(TAG, "offset(x)=%d offset(y)=%d", dev._offsetx, dev._offsety);
	uint16_t _x1 = x;	//+ dev._offsetx;
	uint16_t _x2 = _x1 + size;
	uint16_t _y1 = y;	//+ dev._offsety;
	uint16_t _y2 = _y1;
	//ESP_LOGD(TAG, "_x1=%d _x2=%d _y1=%d _y2=%d", _x1, _x2, _y1, _y2);

	spi_master_write_comm_byte(0x2A);	// set column(x) address
	spi_master_write_addr(_x1, _x2);
	spi_master_write_comm_byte(0x2B);	// set Page(y) address
	spi_master_write_addr(_y1, _y2);
	spi_master_write_comm_byte(0x2C);	// Memory Write
	spi_master_write_colors(colors, size);
}

// Draw rectangle of filling
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End X coordinate
// y2:End Y coordinate
// color:color
void lcdDrawFillRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
	if (x1 >= dev.width)
		return;
	if (x2 >= dev.width)
		x2 = dev.width - 1;
	if (y1 >= dev.height)
		return;
	if (y2 >= dev.height)
		y2 = dev.height - 1;

	//ESP_LOGD(TAG, "offset(x)=%d offset(y)=%d", dev._offsetx, dev._offsety);
	uint16_t _x1 = x1;	//+ dev._offsetx;
	uint16_t _x2 = x2;	//+ dev._offsetx;
	uint16_t _y1 = y1;	//+ dev._offsety;
	uint16_t _y2 = y2;	//+ dev._offsety;

	spi_master_write_comm_byte(0x2A);	// set column(x) address
	spi_master_write_addr(_x1, _x2);
	spi_master_write_comm_byte(0x2B);	// set Page(y) address
	spi_master_write_addr(_y1, _y2);
	spi_master_write_comm_byte(0x2C);	// Memory Write
	for (int i = _x1; i <= _x2; i++)
	{
		uint16_t size = _y2 - _y1 + 1;
		spi_master_write_color(color, size);
	}
}

// Display OFF
void lcdDisplayOff()
{
	spi_master_write_comm_byte(0x28);
}

// Display ON
void lcdDisplayOn()
{
	spi_master_write_comm_byte(0x29);
}

// Display Inversion OFF
void lcdInversionOff()
{
	spi_master_write_comm_byte(0x20);
}

// Display Inversion ON (invierte colores)
void lcdInversionOn()
{
	spi_master_write_comm_byte(0x21);
}

// Change Memory Access Control
void lcdBGRFilter()
{
	spi_master_write_comm_byte(0x36);	//Memory Access Control
	spi_master_write_data_byte(0x00);	//Right top start, RGB color filter panel
}
// Fill screen
// color:color
void lcdFillScreen(uint16_t color)
{
	lcdDrawFillRect(0, 0, dev.width - 1, dev.height - 1, color);
}

// Draw line
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End X coordinate
// y2:End Y coordinate
// color:color 
void lcdDrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
	int i;
	int dx, dy;
	int sx, sy;
	int E;

	/* distance between two points */
	dx = (x2 > x1) ? x2 - x1 : x1 - x2;
	dy = (y2 > y1) ? y2 - y1 : y1 - y2;

	/* direction of two point */
	sx = (x2 > x1) ? 1 : -1;
	sy = (y2 > y1) ? 1 : -1;

	/* inclination < 1 */
	if (dx > dy)
	{
		E = -dx;
		for (i = 0; i <= dx; i++)
		{
			lcdDrawPixel(x1, y1, color);
			x1 += sx;
			E += 2 * dy;
			if (E >= 0)
			{
				y1 += sy;
				E -= 2 * dx;
			}
		}

		/* inclination >= 1 */
	}
	else
	{
		E = -dy;
		for (i = 0; i <= dy; i++)
		{
			lcdDrawPixel(x1, y1, color);
			y1 += sy;
			E += 2 * dx;
			if (E >= 0)
			{
				x1 += sx;
				E -= 2 * dy;
			}
		}
	}
}

// Draw rectangle
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End X coordinate
// y2:End Y coordinate
// color:color
void lcdDrawRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
	lcdDrawLine(x1, y1, x2, y1, color);
	lcdDrawLine(x2, y1, x2, y2, color);
	lcdDrawLine(x2, y2, x1, y2, color);
	lcdDrawLine(x1, y2, x1, y1, color);
}

// Draw rectangle with angle
// xc:Center X coordinate
// yc:Center Y coordinate
// w:Width of rectangle
// h:Height of rectangle
// angle :Angle of rectangle
// color :color

//When the origin is (0, 0), the point (x1, y1) after rotating the point (x, y) by the angle is obtained by the following calculation.
// x1 = x * cos(angle) - y * sin(angle)
// y1 = x * sin(angle) + y * cos(angle)
void lcdDrawRectAngle(uint16_t xc, uint16_t yc, uint16_t w, uint16_t h, uint16_t angle, uint16_t color)
{
	double xd, yd, rd;
	int x1, y1;
	int x2, y2;
	int x3, y3;
	int x4, y4;
	rd = -angle * M_PI / 180.0;
	xd = 0.0 - w / 2;
	yd = h / 2;
	x1 = (int) (xd * cos(rd) - yd * sin(rd) + xc);
	y1 = (int) (xd * sin(rd) + yd * cos(rd) + yc);

	yd = 0.0 - yd;
	x2 = (int) (xd * cos(rd) - yd * sin(rd) + xc);
	y2 = (int) (xd * sin(rd) + yd * cos(rd) + yc);

	xd = w / 2;
	yd = h / 2;
	x3 = (int) (xd * cos(rd) - yd * sin(rd) + xc);
	y3 = (int) (xd * sin(rd) + yd * cos(rd) + yc);

	yd = 0.0 - yd;
	x4 = (int) (xd * cos(rd) - yd * sin(rd) + xc);
	y4 = (int) (xd * sin(rd) + yd * cos(rd) + yc);

	lcdDrawLine(x1, y1, x2, y2, color);
	lcdDrawLine(x1, y1, x3, y3, color);
	lcdDrawLine(x2, y2, x4, y4, color);
	lcdDrawLine(x3, y3, x4, y4, color);
}

// Draw triangle
// xc:Center X coordinate
// yc:Center Y coordinate
// w:Width of triangle
// h:Height of triangle
// angle :Angle of triangle
// color :color

//When the origin is (0, 0), the point (x1, y1) after rotating the point (x, y) by the angle is obtained by the following calculation.
// x1 = x * cos(angle) - y * sin(angle)
// y1 = x * sin(angle) + y * cos(angle)
void lcdDrawTriangle(uint16_t xc, uint16_t yc, uint16_t w, uint16_t h, uint16_t angle, uint16_t color)
{
	double xd, yd, rd;
	int x1, y1;
	int x2, y2;
	int x3, y3;
	rd = -angle * M_PI / 180.0;
	xd = 0.0;
	yd = h / 2;
	x1 = (int) (xd * cos(rd) - yd * sin(rd) + xc);
	y1 = (int) (xd * sin(rd) + yd * cos(rd) + yc);

	xd = w / 2;
	yd = 0.0 - yd;
	x2 = (int) (xd * cos(rd) - yd * sin(rd) + xc);
	y2 = (int) (xd * sin(rd) + yd * cos(rd) + yc);

	xd = 0.0 - w / 2;
	x3 = (int) (xd * cos(rd) - yd * sin(rd) + xc);
	y3 = (int) (xd * sin(rd) + yd * cos(rd) + yc);

	lcdDrawLine(x1, y1, x2, y2, color);
	lcdDrawLine(x1, y1, x3, y3, color);
	lcdDrawLine(x2, y2, x3, y3, color);
}

// Draw circle
// x0:Central X coordinate
// y0:Central Y coordinate
// r:radius
// color:color
void lcdDrawCircle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color)
{
	int x;
	int y;
	int err;
	int old_err;

	x = 0;
	y = -r;
	err = 2 - 2 * r;
	do
	{
		lcdDrawPixel(x0 - x, y0 + y, color);
		lcdDrawPixel(x0 - y, y0 - x, color);
		lcdDrawPixel(x0 + x, y0 - y, color);
		lcdDrawPixel(x0 + y, y0 + x, color);
		if ((old_err = err) <= x)
			err += ++x * 2 + 1;
		if (old_err > y || err > x)
			err += ++y * 2 + 1;
	} while (y < 0);
}

// Draw circle of filling
// x0:Central X coordinate
// y0:Central Y coordinate
// r:radius
// color:color
void lcdDrawFillCircle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color)
{
	int x;
	int y;
	int err;
	int old_err;
	int ChangeX;

	x = 0;
	y = -r;
	err = 2 - 2 * r;
	ChangeX = 1;
	do
	{
		if (ChangeX)
		{
			lcdDrawLine(x0 - x, y0 - y, x0 - x, y0 + y, color);
			lcdDrawLine(x0 + x, y0 - y, x0 + x, y0 + y, color);
		} // endif
		ChangeX = (old_err = err) <= x;
		if (ChangeX)
			err += ++x * 2 + 1;
		if (old_err > y || err > x)
			err += ++y * 2 + 1;
	} while (y <= 0);
}

// Draw rectangle with round corner
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End X coordinate
// y2:End Y coordinate
// r:radius
// color:color
void lcdDrawRoundRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t r, uint16_t color)
{
	int x;
	int y;
	int err;
	int old_err;
	unsigned char temp;

	if (x1 > x2)
	{
		temp = x1;
		x1 = x2;
		x2 = temp;
	} // endif

	if (y1 > y2)
	{
		temp = y1;
		y1 = y2;
		y2 = temp;
	} // endif

	//ESP_LOGD(TAG, "x1=%d x2=%d delta=%d r=%d", x1, x2, x2 - x1, r);
	//ESP_LOGD(TAG, "y1=%d y2=%d delta=%d r=%d", y1, y2, y2 - y1, r);
	if (x2 - x1 < r)
		return; // Add 20190517
	if (y2 - y1 < r)
		return; // Add 20190517

	x = 0;
	y = -r;
	err = 2 - 2 * r;

	do
	{
		if (x)
		{
			lcdDrawPixel(x1 + r - x, y1 + r + y, color);
			lcdDrawPixel(x2 - r + x, y1 + r + y, color);
			lcdDrawPixel(x1 + r - x, y2 - r - y, color);
			lcdDrawPixel(x2 - r + x, y2 - r - y, color);
		} // endif 
		if ((old_err = err) <= x)
			err += ++x * 2 + 1;
		if (old_err > y || err > x)
			err += ++y * 2 + 1;
	} while (y < 0);

	//ESP_LOGD(TAG, "x1+r=%d x2-r=%d", x1 + r, x2 - r);
	lcdDrawLine(x1 + r, y1, x2 - r, y1, color);
	lcdDrawLine(x1 + r, y2, x2 - r, y2, color);
	//ESP_LOGD(TAG, "y1+r=%d y2-r=%d", y1 + r, y2 - r);
	lcdDrawLine(x1, y1 + r, x1, y2 - r, color);
	lcdDrawLine(x2, y1 + r, x2, y2 - r, color);
}

// Draw arrow
// x0:Start X coordinate
// y0:Start Y coordinate
// x1:End X coordinate
// y1:End Y coordinate
// w:Width of the botom
// color:color
// Thanks http://k-hiura.cocolog-nifty.com/blog/2010/11/post-2a62.html
void lcdDrawArrow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t w, uint16_t color)
{
	double Vx = x1 - x0;
	double Vy = y1 - y0;
	double v = sqrt(Vx * Vx + Vy * Vy);
	//printf("v=%f\n",v);
	double Ux = Vx / v;
	double Uy = Vy / v;

	uint16_t L[2], R[2];
	L[0] = x1 - Uy * w - Ux * v;
	L[1] = y1 + Ux * w - Uy * v;
	R[0] = x1 + Uy * w - Ux * v;
	R[1] = y1 - Ux * w - Uy * v;
	//printf("L=%d-%d R=%d-%d\n",L[0],L[1],R[0],R[1]);

	//lcdDrawLine(x0,y0,x1,y1,color);
	lcdDrawLine(x1, y1, L[0], L[1], color);
	lcdDrawLine(x1, y1, R[0], R[1], color);
	lcdDrawLine(L[0], L[1], R[0], R[1], color);
}

// Draw arrow of filling
// x0:Start X coordinate
// y0:Start Y coordinate
// x1:End X coordinate
// y1:End Y coordinate
// w:Width of the botom
// color:color
void lcdDrawFillArrow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t w, uint16_t color)
{
	double Vx = x1 - x0;
	double Vy = y1 - y0;
	double v = sqrt(Vx * Vx + Vy * Vy);
	//printf("v=%f\n",v);
	double Ux = Vx / v;
	double Uy = Vy / v;

	uint16_t L[2], R[2];
	L[0] = x1 - Uy * w - Ux * v;
	L[1] = y1 + Ux * w - Uy * v;
	R[0] = x1 + Uy * w - Ux * v;
	R[1] = y1 - Ux * w - Uy * v;
	//printf("L=%d-%d R=%d-%d\n",L[0],L[1],R[0],R[1]);

	lcdDrawLine(x0, y0, x1, y1, color);
	lcdDrawLine(x1, y1, L[0], L[1], color);
	lcdDrawLine(x1, y1, R[0], R[1], color);
	lcdDrawLine(L[0], L[1], R[0], R[1], color);

	int ww;
	for (ww = w - 1; ww > 0; ww--)
	{
		L[0] = x1 - Uy * ww - Ux * v;
		L[1] = y1 + Ux * ww - Uy * v;
		R[0] = x1 + Uy * ww - Ux * v;
		R[1] = y1 - Ux * ww - Uy * v;
		//printf("Fill>L=%d-%d R=%d-%d\n",L[0],L[1],R[0],R[1]);
		lcdDrawLine(x1, y1, L[0], L[1], color);
		lcdDrawLine(x1, y1, R[0], R[1], color);
	}
}

// Draw ASCII character
// x:X coordinate
// y:Y coordinate
// ascii: ascii code
int lcdDrawChar(uint16_t x, uint16_t y, char ascii)
{
	uint16_t xx, yy, bit, ofs;
	unsigned char pw, ph;
	int h, w;
	uint16_t mask;

	// debo cargar la font en memoria siempre, por eso llamo a esto.
	// y tambien cargo la letra en el buffer.
	GetFontx(dev.font.fx, ascii, &pw, &ph);
	//ESP_LOGD(TAG, "Font %s %s, w=%d h=%d\n", fx->path, fx->opened ? "open" : "close!", pw, ph);

	int16_t xd1 = 0;
	int16_t yd1 = 0;
	int16_t xd2 = 0;
	int16_t yd2 = 0;
	int16_t xss = 0;
	int16_t yss = 0;
	int16_t xsd = 0;
	int16_t ysd = 0;
	int16_t next = 0;
	int16_t x0 = 0;
	int16_t x1 = 0;
	int16_t y0 = 0;
	int16_t y1 = 0;
	if (dev.font.direction == 0)
	{
		xd1 = +1;
		yd1 = +1; //-1;
		xd2 = 0;
		yd2 = 0;
		xss = x;
		yss = y - (ph - 1);
		xsd = 1;
		ysd = 0;
		next = x + pw;

		x0 = x;
		y0 = y - (ph - 1);
		x1 = x + (pw - 1);
		y1 = y;
	}
	else if (dev.font.direction == 2)
	{
		xd1 = -1;
		yd1 = -1; //+1;
		xd2 = 0;
		yd2 = 0;
		xss = x;
		yss = y + ph + 1;
		xsd = 1;
		ysd = 0;
		next = x - pw;

		x0 = x - (pw - 1);
		y0 = y;
		x1 = x;
		y1 = y + (ph - 1);
	}
	else if (dev.font.direction == 1)
	{
		xd1 = 0;
		yd1 = 0;
		xd2 = -1;
		yd2 = +1; //-1;
		xss = x + ph;
		yss = y;
		xsd = 0;
		ysd = 1;
		next = y + pw; //y - pw;

		x0 = x;
		y0 = y;
		x1 = x + (ph - 1);
		y1 = y + (pw - 1);
	}
	else if (dev.font.direction == 3)
	{
		xd1 = 0;
		yd1 = 0;
		xd2 = +1;
		yd2 = -1; //+1;
		xss = x - (ph - 1);
		yss = y;
		xsd = 0;
		ysd = 1;
		next = y - pw; //y + pw;

		x0 = x - (ph - 1);
		y0 = y - (pw - 1);
		x1 = x;
		y1 = y;
	}

	// color de fondo
	if (dev.font.is_fill)
		lcdDrawFillRect(x0, y0, x1, y1, dev.font.bg_color);

	int bits;
	//ESP_LOGD(TAG, "xss=%d yss=%d\n", xss, yss);
	ofs = 0;
	yy = yss;
	xx = xss;
	for (h = 0; h < ph; h++)
	{
		if (xsd)
			xx = xss;
		if (ysd)
			yy = yss;
		//for(w=0;w<(pw/8);w++) {
		bits = pw;
		for (w = 0; w < ((pw + 4) / 8); w++)
		{
			mask = 0x80;
			for (bit = 0; bit < 8; bit++)
			{
				bits--;
				if (bits < 0)
					continue;

				//ESP_LOGD(TAG, "xx=%d yy=%d mask=%02x fonts[%d]=%02x\n", xx, yy, mask, ofs, fonts[ofs]);

				if (dev.font.fx->bmpFont[ofs] & mask)
				{
					lcdDrawPixel(xx, yy, dev.font.color);
				}
				else
				{
					//if (dev._font_fill) lcdDrawPixel( xx, yy, dev._font_fill_color);
				}
				if (h == (ph - 2) && dev.font.is_underline)
					lcdDrawPixel(xx, yy, dev.font.ul_color);
				if (h == (ph - 1) && dev.font.is_underline)
					lcdDrawPixel(xx, yy, dev.font.ul_color);
				xx = xx + xd1;
				yy = yy + yd2;
				mask = mask >> 1;
			}
			ofs++;
		}
		yy = yy + yd1;
		xx = xx + xd2;
	}

	if (next < 0)
		next = 0;
	return next;
}

int lcdDrawString(uint16_t x, uint16_t y, char *ascii)
{
	int length = strlen(ascii);
	//ESP_LOGI(TAG, "length=%d ascii=%s, x=%d y=%d", length, ascii, x, y);

	for (int i = 0; i < length; i++)
	{
		//ESP_LOGD(TAG, "ascii[%d]=%x x=%d y=%d\n", i, ascii[i], x, y);

		if (dev.font.direction == 0 || dev.font.direction == 2)
			x = lcdDrawChar(x, y, ascii[i]);
		else
			y = lcdDrawChar(x, y, ascii[i]);
	}
	dev.font.y = y;
	dev.font.x = x;
	if (dev.font.direction <= 2)
		return x;
	else
		return y;
}

// Draw character using code
// x:X coordinate
// y:Y coordinate
// code: charcter code
int lcdDrawCode(uint16_t x, uint16_t y, uint8_t code)
{
	//ESP_LOGD(TAG, "code=%x x=%d y=%d\n", code, x, y);
	if (dev.font.direction == 0 || dev.font.direction == 2)
		x = lcdDrawChar(x, y, code);
	else
		y = lcdDrawChar(x, y, code);

	if (dev.font.direction <= 2)
		return x;
	else
		return y;
}

// Set font direction
// dir:Direction
void lcdSetFontDirection(uint16_t dir)
{
	dev.font.direction = dir;
}

// Set font filling
// color:fill color
void lcdSetFontFill(uint16_t color)
{
	dev.font.is_fill = true;
	dev.font.bg_color = color;
}

// UnSet font filling
void lcdUnsetFontFill()
{
	dev.font.is_fill = false;
}

// Set font underline
// color:frame color
void lcdSetFontUnderLine(uint16_t color)
{
	dev.font.is_underline = true;
	dev.font.ul_color = color;
}

// UnSet font underline
void lcdUnsetFontUnderLine()
{
	dev.font.is_underline = false;
}

// Backlight OFF
void lcdBacklightOff()
{
	if (dev._bl >= 0)
	{
		gpio_set_level(dev._bl, 0);
	}
}

// Backlight ON
void lcdBacklightOn()
{
	if (dev._bl >= 0)
	{
		gpio_set_level(dev._bl, 1);
	}
}

// Vertical Scrolling Definition
// tfa:Top Fixed Area (normalmente fontHeight)
// vsa:Vertical Scrolling Area (normalmente height - fontHeight)
// bfa:Bottom Fixed Area (0?)
void lcdSetScrollArea(uint16_t tfa, uint16_t vsa, uint16_t bfa)
{
	spi_master_write_comm_byte(0x33);	// Vertical Scrolling Definition
	spi_master_write_data_word(tfa);
	spi_master_write_data_word(vsa);
	spi_master_write_data_word(bfa);
	//spi_master_write_comm_byte( 0x12);	// Partial Mode ON
}

void lcdResetScrollArea(uint16_t vsa)
{
	spi_master_write_comm_byte(0x33);	// Vertical Scrolling Definition
	spi_master_write_data_word(0);
	//spi_master_write_data_word( 0x140);
	spi_master_write_data_word(vsa);
	spi_master_write_data_word(0);
}

// Vertical Scrolling Start Address
// vsp:Vertical Scrolling Start Address
void lcdScroll(uint16_t vsp)
{
	spi_master_write_comm_byte(0x37);	// Vertical Scrolling Start Address
	spi_master_write_data_word(vsp);
}

#define MAX_LEN 3
#define	XPT_START	0x80
#define XPT_XPOS	0x50
#define XPT_YPOS	0x10
#define XPT_8BIT  	0x80
#define XPT_SER		0x04
#define XPT_DEF		0x03

int xptGetit(int cmd)
{
	char rbuf[MAX_LEN];
	char wbuf[MAX_LEN];

	memset(wbuf, 0, sizeof(rbuf));
	memset(rbuf, 0, sizeof(rbuf));
	wbuf[0] = cmd;
	spi_transaction_t SPITransaction;
	esp_err_t ret;

	memset(&SPITransaction, 0, sizeof(spi_transaction_t));
	SPITransaction.length = MAX_LEN * 8;
	SPITransaction.tx_buffer = wbuf;
	SPITransaction.rx_buffer = rbuf;
#if 1
	ret = spi_device_transmit(dev._XPT_Handle, &SPITransaction);
#else
	ret = spi_device_polling_transmit( dev._XPT_Handle, &SPITransaction );
#endif
	assert(ret==ESP_OK);
	//ESP_LOGD(TAG, "rbuf[0]=%02x rbuf[1]=%02x rbuf[2]=%02x", rbuf[0], rbuf[1], rbuf[2]);
	// 12bit Conversion
	//int pos = (rbuf[1]<<8)+rbuf[2];
	int pos = (rbuf[1] << 4) + (rbuf[2] >> 4);
	return (pos);
}

void xptGetxy(int *xp, int *yp)
{
#if 0
	*xp = xptGetit( (XPT_START | XPT_XPOS) );
	*yp = xptGetit( (XPT_START | XPT_YPOS) );
#endif
#if 1
	*xp = xptGetit((XPT_START | XPT_XPOS | XPT_SER));
	*yp = xptGetit((XPT_START | XPT_YPOS | XPT_SER));
#endif
}

/*
 * setea la font a usar
 */
void lcdSetFont(int fontIndex)
{
	dev.font.fx = &fonts[fontIndex];
}

/*
 * setea la font a usar y retorna un puntero a caracteristicas de la font (tamaño, etc)
 * (ojo que no se setea todo lo de la fx)
 */
void lcdSetFontEx(int font_index, FontxFile *fx)
{
	dev.font.fx = &fonts[font_index];
	if (fx)
	{
		GetFontx(dev.font.fx, 0, 0, 0);
		memset(fx, 0, sizeof(FontxFile));
		fx->path = dev.font.fx->path;
		fx->width = dev.font.fx->width;
		fx->height = dev.font.fx->height;
	}
}

void lcdSetFontColor(uint16_t color)
{
	dev.font.color = color;
}

int lcdPrintf(uint16_t x, uint16_t y, const char *format, ...)
{
	va_list argptr;
	char buf[TAM_PRINT_BUF];
	va_start(argptr, format);
	vsprintf(buf, format, argptr);
	int i = lcdDrawString(x, y, buf);
	va_end(argptr);
	return i;
}

/*
 * Ojo, si no se seteo font da null.
 */
FontxFile* lcdFontCaps(uint8_t *fontWidth, uint8_t *fontHeight)
{
	if (dev.font.fx == NULL)
	{
		*fontWidth = *fontHeight = 0;
		return NULL; // no hay font seleccionada!
	}
	return GetFontx(dev.font.fx, 0, fontWidth, fontHeight);
}

void lcdLoadJpg(int x, int y, const char *fileName)
{
	char buf[50];
	sprintf(buf, "%s/%s", fs_root_name, fileName);
	load_jpg(x, y, buf, CONFIG_WIDTH, CONFIG_HEIGHT);
}

void lcdLoadPng(int x, int y, const char *fileName)
{
	char buf[50];
	sprintf(buf, "%s/%s", fs_root_name, fileName);
	load_png(x, y, buf, CONFIG_WIDTH, CONFIG_HEIGHT);
}

void lcdLoadBmp(int x, int y, const char *fileName)
{
	char buf[50];
	sprintf(buf, "%s/%s", fs_root_name, fileName);
	load_bmp(x, y, buf, CONFIG_WIDTH, CONFIG_HEIGHT);
}
