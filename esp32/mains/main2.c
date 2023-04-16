#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/timer.h"
#include "driver/gpio.h"

#define TGROUP (TIMER_GROUP_0)
#define TNUMBER (TIMER_0)
#define TIMER_DIVIDER         (16)  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds
#define timer_interval_sec 1
#define GPIO_OUTPUT_TEST    2 
#define GPIO_OUTPUT_PIN_SEL  (1ULL<<GPIO_OUTPUT_TEST)

bool led = false;
volatile bool x = false;

static bool IRAM_ATTR timer_group_isr_callback(void *args)
{
    BaseType_t high_task_awoken = pdFALSE;
    x = true;
    return high_task_awoken == pdTRUE; // return whether we need to yield at the end of ISR
   
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

    timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = 1,
    };

    timer_init(TGROUP, TNUMBER, &config);

    timer_set_counter_value(TGROUP, TNUMBER, 0);

    timer_set_alarm_value(TGROUP, TNUMBER, timer_interval_sec * TIMER_SCALE);
    timer_enable_intr(TGROUP, TNUMBER);

    timer_isr_callback_add(TGROUP, TNUMBER, timer_group_isr_callback, NULL, 0);

    timer_start(TGROUP, TNUMBER);
    printf("init\n");

    gpio_set_level(GPIO_OUTPUT_TEST, 0);

    while(1){
        vTaskDelay(50 / portTICK_RATE_MS);
        if(x)
        {
            x = false;
        led = !led;
            printf("test\n");
            if (led){
                gpio_set_level(GPIO_OUTPUT_TEST, 0);
                printf("off\n");
            } else {
                gpio_set_level(GPIO_OUTPUT_TEST, 1);
                printf("on\n");
            }
            
        }
    }
}