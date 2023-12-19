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
// wifi
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_spiffs.h"
#include <esp_http_server.h>

// GPIO
#define GPIO_OUTPUT_MOS         13 
#define GPIO_OUTPUT_LED_GR      12
#define GPIO_OUTPUT_LED_RED     14 
#define GPIO_OUTPUT_TEST        0 
#define GPIO_OUTPUT_FAN         25  
#define GPIO_INPUT_SENS_1       26 
#define GPIO_INPUT_SENS_2       27
#define GPIO_INPUT_BUTTON       2 // push button input pin 
#define GPIO_INPUT_MODE_BUTTON  4 // button mode selection pin
#define GPIO_INPUT_MODE_AUTO    16 // automatic mode selection pin
#define GPIO_INPUT_MODE_WIFI    17 // WIFI mode selection pin
#define ESP_INTR_FLAG_DEFAULT   0 
#define GPIO_INPUT_POSEDGE_SEL  ((1ULL<<GPIO_INPUT_BUTTON) | (1ULL<<GPIO_INPUT_SENS_1) | (1ULL<<GPIO_INPUT_SENS_2) | (1ULL<<GPIO_INPUT_MODE_BUTTON) | (1ULL<<GPIO_INPUT_MODE_AUTO) | (1ULL<<GPIO_INPUT_MODE_WIFI))
//#define GPIO_INPUT_ANYEDGE_SEL  ((1ULL<<GPIO_INPUT_SENS_1) | (1ULL<<GPIO_INPUT_SENS_2) | (1ULL<<GPIO_INPUT_MODE_BUTTON) | (1ULL<<GPIO_INPUT_MODE_AUTO) | (1ULL<<GPIO_INPUT_MODE_WIFI))
//#define GPIO_INPUT_DISABLE_SEL  ((1ULL<<GPIO_INPUT_MODE_BUTTON) | (1ULL<<GPIO_INPUT_MODE_AUTO))
#define GPIO_OUTPUT_PIN_SEL     ((1ULL<<GPIO_OUTPUT_MOS) | (1ULL<<GPIO_OUTPUT_LED_GR) | (1ULL<<GPIO_OUTPUT_LED_RED) | (1ULL<<GPIO_OUTPUT_TEST) | (1ULL<<GPIO_OUTPUT_FAN))

extern int adc_max_coil_voltage;
extern int TERM_TEMP_1;
extern int TERM_TEMP_2;

// timers
#define TGROUP (TIMER_GROUP_0)
#define TNUMBER (TIMER_0)
#define TIMER_DIVIDER         (16)  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds
#define coiluptime              0.020 // coil up time in seconds
#define BUTTON_DELAY            0.2 // delay after coil is turned on

volatile bool TIMER_STATE;
volatile bool LEDTEST;
volatile bool TIMER_DELAY_STATE; //Indicates if delay after coil was turned on is counting
volatile bool TIMER_READY;
volatile bool TEMP_OVERLOAD;
volatile bool SPEED_PRINT;
volatile bool PRINT_TEMP;
volatile bool INT_TEST;
extern int INT_TEST_COUNTER;

//ADC
#define DEFAULT_VREF            1100        //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES           64          //Multisampling
#define TERM_BETA               3960.00
#define TERM_T0                 25.00
#define TERM_FAN_UP             35.00 // FAN ON above
#define TERM_FAN_DOWN           32.00 // FAN OFF below
#define TERM_CRIT               45.00 // COIL OFF above
#define TERM_NONCRIT            40 // COIL ON below
#define TERM_R0                 100000.00
#define ADC_1                   (ADC_CHANNEL_6)
#define ADC_2                   (ADC_CHANNEL_4)//7

static esp_adc_cal_characteristics_t *adc_chars;
static const adc_bits_width_t width = ADC_WIDTH_BIT_12;
static const adc_atten_t atten = ADC_ATTEN_DB_11;
static const adc_unit_t unit = ADC_UNIT_1; 

bool ADC_SEL;
extern uint32_t adc_1_reading;
extern uint32_t TERM_1_V;
extern uint32_t TERM_2_V;
extern int adc_interval;

// delay calc
extern uint64_t timeinsens_SC;
extern volatile uint64_t speed_SC;
extern volatile uint64_t speed2_SC;
extern volatile uint64_t delay_SC;
extern volatile uint64_t delay2_SC;
extern volatile int64_t dist_SC;
extern volatile uint64_t speed_avg_SC;

extern double raw;
extern double speed;
extern double speed2;
extern double delay2;
extern double delay;
extern double speed_avg;

// webserver
#define LED_GPIO_PIN 12 
#define INDEX_HTML_PATH "/spiffs/index.html"
char index_html[0x1000];
extern volatile int number;
extern volatile int led_state;
void wifi_init_softap(void);
void initi_web_page_buffer(void);
void start_webserver(void);

extern void IRAM_ATTR gpio_isr_handler(void* arg);