 /* GPIO Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/timer.h"

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

#define GPIO_OUTPUT_MOS    13 
#define GPIO_OUTPUT_LED    12 
#define GPIO_OUTPUT_TEST    0 
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_MOS) | (1ULL<<GPIO_OUTPUT_LED) | (1ULL<<GPIO_OUTPUT_TEST))
#define GPIO_INPUT_SENS     26 
#define GPIO_INPUT_INTPIN_SEL  ((1ULL<<GPIO_INPUT_SENS))
#define GPIO_INPUT_BUTTONPIN_SEL  ((1ULL<<GPIO_INPUT_BUTTON))
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_MODE_BUTTON) | (1ULL<<GPIO_INPUT_MODE_AUTO))
#define GPIO_INPUT_BUTTON   2 // push button input pin
#define GPIO_INPUT_MODE_BUTTON 4 // button mode selection pin
#define GPIO_INPUT_MODE_AUTO 16 // automatic mode selection pin
#define ESP_INTR_FLAG_DEFAULT 0 
#define coiluptime 0.005 // coil up time in seconds

// timer selection
#define TGROUP (TIMER_GROUP_0)
#define TNUMBER (TIMER_0)
//
#define TIMER_DIVIDER         (16)  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds

volatile bool STATE = false;
volatile bool LEDTEST = false;



static bool IRAM_ATTR timer_group_isr_callback(void *args)
{
    BaseType_t high_task_awoken = pdFALSE;
    if (!STATE){ // output to low and set next callback to some long duration later will disable interrupt
        gpio_set_level(GPIO_OUTPUT_MOS, 0);
        //gpio_set_level(GPIO_OUTPUT_LED, 0);
        timer_pause(TGROUP, TNUMBER);
        timer_set_counter_value(TGROUP, TNUMBER, 0);
        timer_group_set_alarm_value_in_isr(TGROUP, TNUMBER, 2 * TIMER_SCALE);
    } else { // output pin to high and set next callback in "coiluptime" time
        gpio_set_level(GPIO_OUTPUT_MOS, 1);
        //gpio_set_level(GPIO_OUTPUT_LED, 1);
        timer_pause(TGROUP, TNUMBER);
        timer_set_counter_value(TGROUP, TNUMBER, 0);
        timer_group_set_alarm_value_in_isr(TGROUP, TNUMBER, coiluptime * TIMER_SCALE);
        timer_start(TGROUP, TNUMBER);
        STATE = false;
        LEDTEST = true;
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
    
    timer_isr_callback_add(group, timer, timer_group_isr_callback, NULL, 0);
}

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint64_t NONTVALUE;
    uint64_t time = 0;
    uint64_t bdiam = 0.02 * TIMER_SCALE;
    uint64_t speed = 0;
    uint64_t distance = 0.1 * TIMER_SCALE;
    uint64_t delay = 0;
    if (gpio_get_level(GPIO_INPUT_SENS) == 1 && gpio_get_level(GPIO_INPUT_MODE_AUTO) == 1) {
        //printf("inp pos edge\n");
        timer_pause(TGROUP, TNUMBER);
        timer_set_counter_value(TGROUP, TNUMBER, 0);
        timer_start(TGROUP, TNUMBER);
    } else if (gpio_get_level(GPIO_INPUT_SENS) == 0 && gpio_get_level(GPIO_INPUT_MODE_AUTO) == 1) {
        //printf("inp neg edge\n");
        timer_pause(TGROUP, TNUMBER);
        timer_get_counter_value(TGROUP, TNUMBER, &NONTVALUE);
        timer_set_counter_value(TGROUP, TNUMBER, 0);
        time = NONTVALUE;
        speed = TIMER_SCALE * bdiam / time;
        delay = TIMER_SCALE * distance / speed;
        timer_set_alarm_value(TGROUP, TNUMBER, delay);
        timer_start(TGROUP, TNUMBER);
        STATE = true;
    } else if (gpio_get_level(GPIO_INPUT_BUTTON) == 1 && gpio_get_level(GPIO_INPUT_MODE_BUTTON) == 1) {
        timer_pause(TGROUP, TNUMBER);
        timer_set_counter_value(TGROUP, TNUMBER, 0);
        timer_set_alarm_value(TGROUP, TNUMBER, distance);
        timer_start(TGROUP, TNUMBER);
        STATE = true;
    }
}

void app_main(void)
{
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
    gpio_set_level(GPIO_OUTPUT_LED, 0);
    gpio_set_level(GPIO_OUTPUT_TEST, 0);

    example_tg_timer_init(TGROUP, TNUMBER, true, 1);
    //example_tg_timer_init(TIMER_GROUP_1, TIMER_0, false, 5);
    while(1) {
        vTaskDelay(200 / portTICK_RATE_MS);
        if (LEDTEST){
            LEDTEST = false;
            gpio_set_level(GPIO_OUTPUT_LED, 1);
            vTaskDelay(250 / portTICK_RATE_MS);
            gpio_set_level(GPIO_OUTPUT_LED, 0);
        }
    }
}