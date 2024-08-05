#include "oled.h"
#include "sh1107.h"
#include "main.h"


void oled_drawloading(void)
{
    sh1107_bitmaps(&dev, 0, -2, logoVUT, 128, 64, false);
}

void oled_init(void)
{
    i2c_master_init(&dev, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_I2C_RESET_GPIO);
    sh1107_init(&dev, 130, 64);
    sh1107_contrast(&dev, 0xff);	
    sh1107_clear_screen(&dev, false);
    sh1107_direction(&dev, DIRECTION180);
}

int oled_drawstats(enum operation mode, int oled_loading_progress)
{

        if (oled_loading_progress < 46 )
        {
            sh1107_bitmaps(&dev, 0, 42+(oled_loading_progress), loadbar, 1, 8, false);
            if (oled_loading_progress == 45)
            {
                sh1107_clear_screen(&dev, false);
            }
            oled_loading_progress++;
        } 
        else if (oled_loading_progress > 45) 
        {
            char string_num[12];
            int oled_speed = speed_SC/TIMER_SCALE*3.6;
            snprintf(string_num, sizeof(string_num), "%d", oled_speed);
            char string_txt[32] = "Speed: ";
            strcat(string_txt, string_num);
            strcat(string_txt, " km/h");
            sh1107_display_text(&dev, 1, 1, string_txt, 16, false);
            
            snprintf(string_num, sizeof(string_num), "%d", term_temp_2);
            strcpy(string_txt, "Board: ");
            strcat(string_txt, string_num);
            strcat(string_txt, " 'C");
            sh1107_display_text(&dev, 3, 1, string_txt, 20, false);

            snprintf(string_num, sizeof(string_num), "%d", term_temp_1);
            strcpy(string_txt, "Coil:  ");
            strcat(string_txt, string_num);
            strcat(string_txt, " 'C");
            sh1107_display_text(&dev, 5, 1, string_txt, 20, false);

            switch (mode){
                case button:
                    sh1107_display_text(&dev, 7, 2, "MODE: MANUAL ", 16, false);
                break;
                case asist:
                    sh1107_display_text(&dev, 7, 2, "MODE: ASSIST  ", 16, false);
                break;
                case automatic:
                    sh1107_display_text(&dev, 7, 2, "MODE: AUTO  ", 16, false);
                break;
            }
        } 
        return oled_loading_progress;
}