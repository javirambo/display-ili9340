/*
 FsBuffer.c
 Guarda archivos en el File System a modo de buffer.
 Usa varios archivos para guardar en forma circular.
 Si existe una micro-SD la usa, sino abre en el FS de la flash del ESP32.

 Version 2
 JJTeam - 2021/2022
 */

#include <esp_log.h>
#include <string.h>
#include <esp_system.h>
#include "fsBuffer.h"
#include "fsTools.h"
#include "iniFile.h"
#include "sdkconfig.h"

static const char *TAG = "fsBuffer";

#define KEY_INDEX 	"fileIndexActual"
#define KEY_TOTAL 	"totalFiles"
#define BUF_NAME 	"buf.XX"
#define CFG_NAME 	"buffers.ini"

static int8_t fileIndexActual; // archivo que se esta escribiendo
static char *bufFileName;      // full path a filename, template (XX) indica el numero de file.
static char *cfgFileName;      // full path a configuraciones

static char* fsBuffer_getFileName(char *buf, int index)
{
	// ejemplo, aca se guardan el log:
	// bufFileName="javier/buf.XX"
	// cambio el XX por el numero de log.
	int len = strlen(buf);
	buf[len - 2] = '0' + (index / 10);
	buf[len - 1] = '0' + (index % 10);
	return buf;
}

// guarda cosas en flash (por si un reset)
static void fsBuffer_storeConfig()
{
	ini_file_t ini;
	ini.create_if_not_exists = true;
	ESP_LOGD(TAG, "store");
	ini_file_open(&ini, cfgFileName);
	ESP_LOGD(TAG, "name=%s fp=%s", ini.name, (ini.fp==NULL?"null":"fp"));
	ini_file_seti(&ini, KEY_INDEX, fileIndexActual);
	ESP_LOGD(TAG, "set 1");
	// dejo constancia en el archivo de configuracion, como leer los logs,
	// por las dudas que se levanten desde una PC.
	ini_file_seti(&ini, KEY_TOTAL, CONFIG_FS_LOG_CANT_FILES);
	ESP_LOGD(TAG, "set 2");
	ini_file_close(&ini);
	ESP_LOGD(TAG, "close");
}

// Cambio de archivo. Voy al siguiente circularmente.
static void fsBuffer_nextFile()
{
	fileIndexActual++;
	if (fileIndexActual == CONFIG_FS_LOG_CANT_FILES)
	{
		fileIndexActual = 0;
		ESP_LOGW(TAG, "Ya se usaron todos los archivos de log. Se rota al primero");
	}
	fsBuffer_storeConfig();

	// ejemplo: /log/buf.01
	bufFileName = fsBuffer_getFileName(bufFileName, fileIndexActual);
	remove(bufFileName);
}

// crea el archivo a usar
static void fsBuffer_initFile()
{
	ini_file_t ini;
	ini_file_open(&ini, cfgFileName);
	fileIndexActual = ini_file_geti(&ini, KEY_INDEX, -1);
	ini_file_close(&ini);

	// no existe el archivo de configuracion? inicializo el buffer:
	if (fileIndexActual == -1)
		fsBuffer_nextFile();    // dejo preparado el nombre a usar.
	else
		bufFileName = fsBuffer_getFileName(bufFileName, fileIndexActual);
}

/**
 * Se necesita configurar el tamaño de los archivos (en bytes),
 * la cantidad de archivos,
 * y el nombre que distingue los diferentes buffers que puedan existir.
 *
 * Se grabara en la micro-SD si la memoria esta disponible, sino usara la flash del ESP32.
 *
 * El folder es una carpeta para separar varios posibles FsBuffers.
 *
 * buffer_name sin / al principio !!
 */
void fsBuffer_init(const char *buffer_name)
{
	// abro cualquier FS (sd-card o flash)
	fs_init();

	// le saco el / al principio
	if (*buffer_name == '/')
		buffer_name++;

	cfgFileName = (char*) malloc(1 + strlen(buffer_name) + strlen(CFG_NAME));
	sprintf(cfgFileName, "%s/%s", buffer_name, CFG_NAME);
	bufFileName = (char*) malloc(1 + strlen(buffer_name) + strlen(BUF_NAME));
	sprintf(bufFileName, "%s/%s", buffer_name, BUF_NAME);

	fs_mkdir(buffer_name);
	fsBuffer_initFile();
}

/**
 * Agrega texto al archivo actual.
 * Si se excede el tamaño máximo, sigue con otro archivo.
 */
size_t fsBuffer_write(const char *txt, size_t len)
{
	size_t size = 0;
	FILE *f = fs_open_file(bufFileName, "a");
	if (f)
	{
		size = fwrite(txt, len, 1, f);
		fflush(f);
		int filesize = f->_lbfsize;
		fclose(f);
		// si se excede el tamaño del archivo, cambia a siguiente.
		if (filesize > CONFIG_FS_LOG_FILE_SIZE)
			fsBuffer_nextFile();
	}
	else
		ESP_LOGW(TAG, "Can't write log %s", bufFileName);

	return size;
}

// abre un archivo, y por cada linea llama el callback.
void fsBuffer_forEachLineFromFile(const char *filename, ForEachLineCallback callback)
{
	FILE *f = fs_open_file(filename, "r");
	if (f)
	{
		char buf[500], *line;
		while ((line = fgets(buf, sizeof(buf), f)) != 0)
			callback(line);
		fclose(f);
	}
	else
		ESP_LOGE(TAG, "No pude abrir el archivo %s", filename);
}

// Recorre todas las lineas de todos los archivos:
void fsBuffer_forEachLine(ForEachLineCallback callback)
{
	// Ejemplo: 2-3-0-1
	// (si el actual es el 2, empiezo con el 3 (el mas viejo) y sigo en forma circular)
	int index = fileIndexActual;
	// no quiero que un proceso paralelo me siga escribiendo el los files que estos cambiando temporalmente aca.
	char *file = strdup(bufFileName);
	do
	{
		file = fsBuffer_getFileName(file, fileIndexActual);
		fsBuffer_forEachLineFromFile((const char*) file, callback);
		if (++index == CONFIG_FS_LOG_CANT_FILES)
			index = 0;
	} while (index != fileIndexActual);
	free(file);
}

// elimina todos los archivos!
void fsBuffer_clear()
{
	for (size_t i = 0; i < CONFIG_FS_LOG_CANT_FILES; i++)
	{
		bufFileName = fsBuffer_getFileName(bufFileName, i);
		remove(bufFileName);
	}
	fileIndexActual = -1;
	fsBuffer_nextFile();
	ESP_LOGW(TAG, "ALL BUFFERS DELETED!");
}
