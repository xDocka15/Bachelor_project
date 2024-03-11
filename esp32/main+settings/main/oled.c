#include "oled.h"
#include "sh1107.h"
#include "main_def.h"


void oled_drawloading(void){
    sh1107_bitmaps(&dev, 0, -2, logoVUT, 128, 64, false);
}

void oled_init(void){
    i2c_master_init(&dev, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_I2C_RESET_GPIO);
    sh1107_init(&dev, 130, 64);
    sh1107_contrast(&dev, 0xff);	
    sh1107_clear_screen(&dev, false);
    sh1107_direction(&dev, DIRECTION180);
}

int oled_drawstats(enum OPERATION MODE, int oled_loading_progress){

        if (oled_loading_progress < 46 ){
            sh1107_bitmaps(&dev, 0, 42+(oled_loading_progress), loadbar, 1, 8, false);
            if (oled_loading_progress == 45){
                sh1107_clear_screen(&dev, false);
            }
            oled_loading_progress++;
        } else  if (oled_loading_progress > 45) {
            sh1107_display_text(&dev, 0, 15, "W", 16, false);

            char string_num[12];
            int oled_speed = speed/TIMER_SCALE*3.6;
            snprintf(string_num, sizeof(string_num), "%d", oled_speed);
            char string_txt[32] = "Speed: ";
            strcat(string_txt, string_num);
            strcat(string_txt, " km/h");
            sh1107_display_text(&dev, 1, 1, string_txt, 16, false);
            
            snprintf(string_num, sizeof(string_num), "%d", TERM_TEMP_2);
            strcpy(string_txt, "Board: ");
            strcat(string_txt, string_num);
            strcat(string_txt, " 'C");
            sh1107_display_text(&dev, 3, 1, string_txt, 20, false);

            snprintf(string_num, sizeof(string_num), "%d", TERM_TEMP_1);
            strcpy(string_txt, "Coil:  ");
            strcat(string_txt, string_num);
            strcat(string_txt, " 'C");
            sh1107_display_text(&dev, 5, 1, string_txt, 20, false);

            switch (MODE){
                case BUTTON:
                    sh1107_display_text(&dev, 7, 2, "MODE: BUTTON ", 16, false);
                    // printf("button\n");
                break;
                case AUTO:
                    sh1107_display_text(&dev, 7, 2, "MODE: AUTO  ", 16, false);
                    // printf("auto\n");
                break;
                case WIFI:
                    sh1107_display_text(&dev, 7, 2, "MODE: WIFI  ", 16, false);
                    // printf("wifi\n");
                break;
            }
        } return oled_loading_progress;



}