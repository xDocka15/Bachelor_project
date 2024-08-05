#pragma once

//general
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/timer.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "nvs_flash.h"


// GPIO
#define GPIO_OUTPUT_MOS         13 
#define GPIO_OUTPUT_LED_GR      19 // board 12 outside 19
#define GPIO_OUTPUT_LED_RED     18 // 14
#define GPIO_OUTPUT_FAN         25  
#define GPIO_INPUT_SENS_1       26 
#define GPIO_INPUT_SENS_2       27
#define GPIO_INPUT_BUTTON       17 // push button 
#define WIFI_INPUT_BUTTON       1 // wifi calls isr with this number
#define GPIO_INPUT_MODE_BUTTON  16 // button mode selection pin
#define GPIO_INPUT_MODE_AUTO    4 // automatic mode selection pin
#define ESP_INTR_FLAG_DEFAULT   0 
#define GPIO_INPUT_POSEDGE_SEL  ((1ULL<<GPIO_INPUT_SENS_1) | (1ULL<<GPIO_INPUT_SENS_2))
#define GPIO_INPUT_PULLUP_SEL     ((1ULL<<GPIO_INPUT_BUTTON) | (1ULL<<GPIO_INPUT_MODE_BUTTON) | (1ULL<<GPIO_INPUT_MODE_AUTO))
#define GPIO_OUTPUT_PIN_SEL     ((1ULL<<GPIO_OUTPUT_MOS) | (1ULL<<GPIO_OUTPUT_LED_GR) | (1ULL<<GPIO_OUTPUT_LED_RED) | (1ULL<<GPIO_OUTPUT_FAN))


typedef enum  operation {
    button = 1,
    asist = 2,
    automatic = 3
};
extern enum operation mode;

typedef enum ball_position {
    other,
    between_sensors,
    before_coil,
    after_coil
};

extern int adc_max_coil_voltage;
extern int term_temp_1;
extern int term_temp_2;

// timers
#define TGROUP (TIMER_GROUP_0)
#define TNUMBER (TIMER_0)
#define TIMER_DIVIDER         (16)  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds
#define coiluptime              0.020 // coil up time in seconds
#define BUTTON_DELAY            0.5 // delay after coil is turned on

extern double speed_SC;

extern void IRAM_ATTR gpio_isr_handler(void* arg);