idf_component_register(SRCS "ili9341.c"	"decode_jpeg.c" "decode_png.c" "pngle.c" "fontx.c" "bmpfile.c"
					INCLUDE_DIRS "./include"
					#REQUIRES "esp_spiffs"
					REQUIRES "spiffs")
