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
#define GPIO_OUTPUT_TEST    4 
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_MOS) | (1ULL<<GPIO_OUTPUT_LED) | (1ULL<<GPIO_OUTPUT_TEST))
#define GPIO_INPUT     26
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT))
#define ESP_INTR_FLAG_DEFAULT 0

#define TGROUP (TIMER_GROUP_0)
#define TNUMBER (TIMER_0)
#define TIMER_DIVIDER         (16)  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds

volatile uint64_t TVALUE = 0;

typedef struct {
    int timer_group;
    int timer_idx;
    int alarm_interval;
    bool auto_reload;
} example_timer_info_t;

/**
 * @brief A sample structure to pass events from the timer ISR to task
 *
 */
typedef struct {
    example_timer_info_t info;
    uint64_t timer_counter_value;
} example_timer_event_t;

static bool IRAM_ATTR timer_group_isr_callback(void *args)
{
    BaseType_t high_task_awoken = pdFALSE;

    //TVALUE = timer_group_get_counter_value_in_isr(TGROUP, TNUMBER);

    //gpio_set_level(GPIO_OUTPUT_MOS, 0);
    //gpio_set_level(GPIO_OUTPUT_LED, 0);
    //gpio_set_level(GPIO_OUTPUT_TEST, 0);

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

    //timer_start(group, timer);
    //printf("timer init\n");
}


static xQueueHandle gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, NULL, NULL);
}

static void gpio_task_example(void* arg)
{
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            if (gpio_get_level(GPIO_INPUT) == 1){
                printf("inp pos edge\n");

                timer_set_counter_value(TGROUP, TNUMBER, 0);
                timer_start(TGROUP, TNUMBER);
            } else {
                printf("inp neg edge\n");
                timer_pause(TGROUP, TNUMBER);
                timer_get_counter_value(TGROUP, TNUMBER, TVALUE);
                timer_set_counter_value(TGROUP, TNUMBER, 0);

                timer_set_alarm_value(TGROUP, TNUMBER, TVALUE * 2);
                timer_set_counter_value(TGROUP, TNUMBER, 0);
                timer_start(TGROUP, TNUMBER);
                gpio_set_level(GPIO_OUTPUT_MOS, 1);
                gpio_set_level(GPIO_OUTPUT_LED, 1);
                gpio_set_level(GPIO_OUTPUT_TEST, 1);
            }
        }
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
    //bit mask of the pins that you want to set,e.g.GPIO18/19
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
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-down mode
    io_conf.pull_down_en = 1;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT, gpio_isr_handler, (void*) NULL);

    //printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());

    //int cnt = 0;
    gpio_set_level(GPIO_OUTPUT_MOS, 0);
    gpio_set_level(GPIO_OUTPUT_LED, 0);
    gpio_set_level(GPIO_OUTPUT_TEST, 0);

    example_tg_timer_init(TGROUP, TNUMBER, false, 1);
    //example_tg_timer_init(TIMER_GROUP_1, TIMER_0, false, 5);
    while(1) {
        //printf("cnt: %d\n", cnt++);
        vTaskDelay(1000 / portTICK_RATE_MS);
        //gpio_set_level(GPIO_OUTPUT_IO_0, cnt % 2);
    }
}