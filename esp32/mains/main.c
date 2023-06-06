 /* GPIO Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
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


/**
 * Brief:
 * This test code shows how to configure gpio and how to use gpio interrupt.
 *
 * GPIO status:
 * GPIO18: output
 * GPIO19: output
 * GPIO4:  input, pulled up, interrupt from rising edge and falling edge
 * GPIO5:  input, pulled up, interrupt from rising edge.
 *
 * Test:
 * Connect GPIO18 with GPIO4
 * Connect GPIO19 with GPIO5
 * Generate pulses on GPIO18/19, that triggers interrupt on GPIO4/5
 *
 */

#define GPIO_OUTPUT_MOS       13 
#define GPIO_OUTPUT_LED_GR    12
#define GPIO_OUTPUT_LED_RED   14 
#define GPIO_OUTPUT_TEST       0 
#define GPIO_OUTPUT_FAN       25  
#define GPIO_OUTPUT_PIN_SEL    ((1ULL<<GPIO_OUTPUT_MOS) | (1ULL<<GPIO_OUTPUT_LED_GR) | (1ULL<<GPIO_OUTPUT_LED_RED) | (1ULL<<GPIO_OUTPUT_TEST) | (1ULL<<GPIO_OUTPUT_FAN))
#define GPIO_INPUT_SENS       26 
#define GPIO_INPUT_INTPIN_SEL  ((1ULL<<GPIO_INPUT_SENS))
#define GPIO_INPUT_BUTTONPIN_SEL ((1ULL<<GPIO_INPUT_BUTTON))
#define GPIO_INPUT_PIN_SEL      ((1ULL<<GPIO_INPUT_MODE_BUTTON) | (1ULL<<GPIO_INPUT_MODE_AUTO))
#define GPIO_INPUT_BUTTON       2 // push button input pin
#define GPIO_INPUT_MODE_BUTTON  4 // button mode selection pin
#define GPIO_INPUT_MODE_AUTO   16 // automatic mode selection pin
#define ESP_INTR_FLAG_DEFAULT   0 
#define coiluptime              0.020 // coil up time in seconds
#define BUTTON_DELAY            0.5 // delay after coil is turned on

// ADC
#define DEFAULT_VREF            1100        //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES           64          //Multisampling
#define TERM_BETA               3960.00
#define TERM_T0                 25.00
#define TERM_T1                 50.00
#define TERM_R0                 100000.00
int TERM_R1 = TERM_R0*exp(TERM_BETA*(1/(TERM_T1+273) - 1/(TERM_T0+273))); // resistance for T1
int adc_max_coil_voltage = 2000;
int TERM_TEMP = 0;

//
// timer selection
#define TGROUP (TIMER_GROUP_0)
#define TNUMBER (TIMER_0)
//
#define TIMER_DIVIDER         (16)  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds

volatile bool TIMER_STATE = false;
volatile bool LEDTEST = false;
volatile bool TIMER_DELAY_STATE = false; //Indicates if delay after coil was turned on is counting
volatile bool TEMP_OVERLOAD = false;

//ADC
static esp_adc_cal_characteristics_t *adc_chars;
    static const adc_bits_width_t width = ADC_WIDTH_BIT_12;
    static const adc_atten_t atten = ADC_ATTEN_DB_11;
    static const adc_unit_t unit = ADC_UNIT_1; 

#define ADC_1   (ADC_CHANNEL_6)
#define ADC_2   (ADC_CHANNEL_7)
bool ADC_SEL = true;
uint32_t adc_1_reading = 0;
uint32_t TERM_1_V = 0;
int adc_interval = 0;
volatile uint64_t speed_SC = 0;



static bool IRAM_ATTR timer_1_isr_callback(void *args)
{
    BaseType_t high_task_awoken = pdFALSE;
    if (!TIMER_STATE && !TIMER_DELAY_STATE){ // output to low and set next callback to some long duration later will disable interrupt
        TIMER_DELAY_STATE = true;
        gpio_set_level(GPIO_OUTPUT_MOS, 0);
        timer_pause(TGROUP, TNUMBER);
        timer_set_counter_value(TGROUP, TNUMBER, 0);
        timer_group_set_alarm_value_in_isr(TGROUP, TNUMBER, BUTTON_DELAY * TIMER_SCALE);
        timer_start(TGROUP, TNUMBER);
    } else if (TIMER_STATE && !TIMER_DELAY_STATE){ // output pin to high and set next callback in "coiluptime" time
        gpio_set_level(GPIO_OUTPUT_MOS, 1);
        timer_pause(TGROUP, TNUMBER);
        timer_set_counter_value(TGROUP, TNUMBER, 0);
        timer_group_set_alarm_value_in_isr(TGROUP, TNUMBER, coiluptime * TIMER_SCALE);
        timer_start(TGROUP, TNUMBER);
        TIMER_STATE = false;
        LEDTEST = true;
    } else if (TIMER_DELAY_STATE){
        TIMER_DELAY_STATE = false;
        timer_pause(TGROUP, TNUMBER);
        timer_set_counter_value(TGROUP, TNUMBER, 0);
        timer_group_set_alarm_value_in_isr(TGROUP, TNUMBER, 2 * TIMER_SCALE);
    }
    return high_task_awoken == pdTRUE; // return whether we need to yield at the end of ISR
}

/**
 * @brief Initialize selected timer of timer group
 *
 * @param group Timer Group number, index from 0
 * @param timer timer ID, index from 0
 * @param auto_reload whether auto-reload on alarm event
 * @param timer_interval_sec interval of alarm
 */
static void example_tg_timer_init(int group, int timer, bool auto_reload, int timer_interval_sec)
{
    /* Select and initialize basic parameters of the timer */
    timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = auto_reload,
    }; // default clock source is APB

    timer_init(group, timer, &config);

    /* Timer's counter will initially start from value below.
       Also, if auto_reload is set, this value will be automatically reload on alarm */
    timer_set_counter_value(group, timer, 0);

    /* Configure the alarm value and the interrupt on alarm. */
    timer_set_alarm_value(group, timer, timer_interval_sec * TIMER_SCALE);
    timer_enable_intr(group, timer);
    
    timer_isr_callback_add(group, timer, timer_1_isr_callback, NULL, 0);
}

static void ADC_init(int channel)
{
    adc1_config_width(width);
    adc1_config_channel_atten(channel, atten);
    esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, adc_chars);
}

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint64_t timeinsens_SC;
    //uint64_t time = 0;
    uint64_t bdiam_SC = 0.02 * TIMER_SCALE;
    uint64_t distance_SC = 0.080 * TIMER_SCALE;
    uint64_t coillength_SC = 0.04 * TIMER_SCALE;
    uint64_t delay_SC = 0;
    uint64_t delay2_SC = 0;
    if (!TIMER_DELAY_STATE && !TEMP_OVERLOAD) {
        if (gpio_get_level(GPIO_INPUT_SENS) == 1 && gpio_get_level(GPIO_INPUT_MODE_AUTO) == 1) {
            timer_pause(TGROUP, TNUMBER);
            timer_set_counter_value(TGROUP, TNUMBER, 0);
            timer_set_alarm_value(TGROUP, TNUMBER, 1 * TIMER_SCALE);
            timer_start(TGROUP, TNUMBER);
        } else if (gpio_get_level(GPIO_INPUT_SENS) == 0 && gpio_get_level(GPIO_INPUT_MODE_AUTO) == 1) {
            timer_pause(TGROUP, TNUMBER);
            timer_get_counter_value(TGROUP, TNUMBER, &timeinsens_SC);
            timer_set_counter_value(TGROUP, TNUMBER, 0);
            speed_SC = TIMER_SCALE * bdiam_SC / timeinsens_SC;
            delay_SC = TIMER_SCALE * distance_SC / speed_SC;
            delay2_SC = delay_SC - coiluptime * TIMER_SCALE + (coillength_SC/speed_SC);
            timer_set_alarm_value(TGROUP, TNUMBER, delay2_SC);
            timer_start(TGROUP, TNUMBER);
            TIMER_STATE = true;
        } else if (gpio_get_level(GPIO_INPUT_BUTTON) == 1 && gpio_get_level(GPIO_INPUT_MODE_BUTTON) == 1) {
            timer_pause(TGROUP, TNUMBER);
            timer_set_counter_value(TGROUP, TNUMBER, 0);
            timer_set_alarm_value(TGROUP, TNUMBER, distance_SC);
            timer_start(TGROUP, TNUMBER);
            TIMER_STATE = true;
        }
    }
}

void app_main(void)
{
//GPIO
    //zero-initialize the config structure.
    gpio_config_t io_conf = {};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    //interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_INTPIN_SEL;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-down mode
    io_conf.pull_down_en = 1;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.pin_bit_mask = GPIO_INPUT_BUTTONPIN_SEL;
    gpio_config(&io_conf);

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    gpio_config(&io_conf);
    
    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_SENS, gpio_isr_handler, (void*) NULL);
    gpio_isr_handler_add(GPIO_INPUT_BUTTON, gpio_isr_handler, (void*) NULL);

    gpio_set_level(GPIO_OUTPUT_MOS, 0);
    gpio_set_level(GPIO_OUTPUT_LED_GR, 0);
    gpio_set_level(GPIO_OUTPUT_TEST, 0);

    example_tg_timer_init(TGROUP, TNUMBER, true, 1);
    //example_tg_timer_init(TIMER_GROUP_1, TIMER_0, false, 5);

//ADC

    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    ADC_init(ADC_1);
    ADC_init(ADC_2);
    
    double speed_max = 0.01;
    while(1) {
        vTaskDelay(250 / portTICK_RATE_MS);
        
        double speed = speed_SC;
        if (speed > speed_max){
            speed_max = speed;
            
        }
        //printf("speed %f \t max speed %f \n", speed/TIMER_SCALE, speed_max/TIMER_SCALE);

        bool SENS_PRINT = false;
        if (SENS_PRINT){
            if (gpio_get_level(GPIO_INPUT_SENS) == 1){
                printf("in \n");
            } else {
                printf("out \n");
            }
        }   

        if (LEDTEST){
            LEDTEST = false;
            gpio_set_level(GPIO_OUTPUT_LED_GR, 1);
            vTaskDelay(250 / portTICK_RATE_MS);
            gpio_set_level(GPIO_OUTPUT_LED_GR, 0);
        }
        adc_interval ++;
        if (adc_interval == 5) 
        {
            adc_interval = 0;
            if (ADC_SEL)
            {
                adc_1_reading = adc1_get_raw((adc1_channel_t)ADC_1);
                printf(" TERM_1 %d\n",TERM_TEMP);
                ADC_SEL = false;
            } else {
                adc_1_reading = adc1_get_raw((adc1_channel_t)ADC_2);
                printf(" COIL TEMP %d\n",TERM_TEMP);
                ADC_SEL = true;
            }
            adc_max_coil_voltage = 1000.00*5*TERM_R1/(TERM_R1+56000);
            TERM_1_V = esp_adc_cal_raw_to_voltage(adc_1_reading, adc_chars);
            int TERM_1_R1 = TERM_1_V/1000.00*56000.00/(5-TERM_1_V/1000.00);
            TERM_TEMP = 1/((1/(TERM_T0+273))+(1/TERM_BETA)*log(TERM_1_R1/TERM_R0))-273;
            if (TERM_1_V < adc_max_coil_voltage){
                TEMP_OVERLOAD = true;
                gpio_set_level(GPIO_OUTPUT_LED_RED, 1);
            } else {
                TEMP_OVERLOAD = false;
                gpio_set_level(GPIO_OUTPUT_LED_RED, 0);
            } 
        }
    }
}