idf_component_register(SRCS "ADC.c" "main.c" "softAP.c" "webserver.c" "oled.c"
                    INCLUDE_DIRS ".")

spiffs_create_partition_image(storage ../data FLASH_IN_PROJECT)