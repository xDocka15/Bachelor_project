#include "main.h"
#include "sh1107.h"
#include "oled.h"
#include "webserver.h"
#include "ADC.h"

//**************************************************************

volatile bool timer_state = false;
volatile bool timer_ready = true;
volatile bool temp_overload = false;

int term_temp_1 = 0;
int term_temp_2 = 0;
uint32_t adc_1_reading = 0;
uint32_t term_1_v = 0;
uint32_t term_2_v = 0;
int adc_interval = 0;
//

volatile uint64_t sens_distance_SC = 0.130 * TIMER_SCALE;  
volatile uint64_t dist_to_coil_SC = 0.120 * TIMER_SCALE; // distance to the center
uint64_t timeinsens_SC; // musi byt int64
double speed_SC = 0;
volatile double speed2_SC = 0;
volatile double delay_SC = 0;
volatile double delay2_SC = 0;
volatile int64_t dist_SC = 0;
volatile double speed_avg_SC = 0;
double speed_max = 0.01;


double prev_speed = 0;

int speed_reset_counter = 0;

//**************************************************************
volatile bool asist_en = false;

enum operation mode = button;

enum ball_position position = other;

static void IRAM_ATTR mode_check(void* arg)  
{
    if (gpio_get_level(GPIO_INPUT_MODE_BUTTON) == 0 && gpio_get_level(GPIO_INPUT_MODE_AUTO) == 1)
    {
        mode = button;
        gpio_intr_enable(GPIO_INPUT_BUTTON);
    } else if (gpio_get_level(GPIO_INPUT_MODE_BUTTON) == 1 && gpio_get_level(GPIO_INPUT_MODE_AUTO) == 0) 
    {
        mode = automatic;
        gpio_intr_disable(GPIO_INPUT_BUTTON);
    } else 
    { // button = 0 auto = 0 assit = 0
        mode = asist;
        gpio_intr_enable(GPIO_INPUT_BUTTON);
    }
}

static bool IRAM_ATTR timer_1_isr_callback(void *args)
{
    BaseType_t high_task_awoken = pdFALSE;

    if (timer_state && position != after_coil)
    { // sepnuti civky
        if (asist_en && mode == asist)
        {
            gpio_set_level(GPIO_OUTPUT_MOS, 1);
        } else if (mode != asist) 
        {
            gpio_set_level(GPIO_OUTPUT_MOS, 1);
        }
        asist_en = false;
        timer_pause(TGROUP, TNUMBER);
        timer_set_counter_value(TGROUP, TNUMBER, 0);
        timer_group_set_alarm_value_in_isr(TGROUP, TNUMBER, coiluptime * TIMER_SCALE);
        timer_start(TGROUP, TNUMBER);
        timer_ready = false;
        timer_state = false;
        gpio_set_level(GPIO_OUTPUT_LED_GR, 0);

    } else if (!timer_state && position != after_coil)
    { //vypnuti civky
        position = after_coil;
        gpio_set_level(GPIO_OUTPUT_MOS, 0);
        timer_pause(TGROUP, TNUMBER);
        timer_set_counter_value(TGROUP, TNUMBER, 0);
        timer_group_set_alarm_value_in_isr(TGROUP, TNUMBER, BUTTON_DELAY * TIMER_SCALE);
        timer_start(TGROUP, TNUMBER);
        timer_ready = false;
    } else if (position == after_coil) 
    {
        position = other;
        timer_ready = true;
        timer_pause(TGROUP, TNUMBER);
        timer_set_counter_value(TGROUP, TNUMBER, 0);
        timer_group_set_alarm_value_in_isr(TGROUP, TNUMBER, 10 * TIMER_SCALE);
        gpio_set_level(GPIO_OUTPUT_LED_GR, 1);
    }
    return high_task_awoken == pdTRUE; // return whether we need to yield at the end of ISR
}

static void example_tg_timer_init(int group, int timer, bool auto_reload, int timer_interval_sec)
{
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

volatile int pin = 0;

void IRAM_ATTR gpio_isr_handler(void* arg)
{
    pin = (uint32_t) arg;

    if (position != after_coil && !temp_overload && timer_ready) 
    {
        if (mode == asist && (pin == WIFI_INPUT_BUTTON || pin == GPIO_INPUT_BUTTON) && position == before_coil){
            asist_en = true;
        }

        if ((((pin == WIFI_INPUT_BUTTON || pin == GPIO_INPUT_BUTTON) && (mode == button))) && position != after_coil)
        { 
            timer_state = true;
            asist_en = true; 
            timer_pause(TGROUP, TNUMBER);
            timer_set_counter_value(TGROUP, TNUMBER, 0);
            timer_set_alarm_value(TGROUP, TNUMBER, 0.005 * TIMER_SCALE);
            timer_start(TGROUP, TNUMBER);
        }
  
        if (pin == GPIO_INPUT_SENS_1) 
        { // (kulicka prisla do sensoru 1) reset timeru pro mereni delky zakryti senzoru
            position = between_sensors;
            timer_pause(TGROUP, TNUMBER);
            timer_set_counter_value(TGROUP, TNUMBER, 0);
            timer_set_alarm_value(TGROUP, TNUMBER, 10 * TIMER_SCALE);
            timer_start(TGROUP, TNUMBER);
        } else if (pin == GPIO_INPUT_SENS_2 && position == between_sensors) 
        { // kulicka opustila sensor 2
            position = before_coil;
            delay_SC = 0; 
            delay2_SC = 0;
            timer_pause(TGROUP, TNUMBER);
            timer_get_counter_value(TGROUP, TNUMBER, &timeinsens_SC);
            if (timeinsens_SC != 0)
            {
                speed_SC = TIMER_SCALE * sens_distance_SC / timeinsens_SC;
                dist_SC = log((speed_SC*1.000/TIMER_SCALE)/1.632)/(-0.5) * TIMER_SCALE + dist_to_coil_SC;
                speed2_SC = 1.632*exp(-0.5 * dist_SC/TIMER_SCALE) * TIMER_SCALE;
                speed_avg_SC = (speed_SC + speed2_SC)/2;
                delay_SC = TIMER_SCALE * dist_to_coil_SC / speed_avg_SC;
                if ((delay_SC > coiluptime * TIMER_SCALE) && (mode == automatic || mode == asist))
                {  //pokud coiluptime > delay_SC >> skip pulzu
                    timer_state = true;
                    delay2_SC = delay_SC - (coiluptime * TIMER_SCALE);
                    timer_set_counter_value(TGROUP, TNUMBER, 0);              
                    timer_set_alarm_value(TGROUP, TNUMBER, delay2_SC);
                    timer_start(TGROUP, TNUMBER);
                } else 
                {
                    timer_set_counter_value(TGROUP, TNUMBER, 0);
                    timer_set_alarm_value(TGROUP, TNUMBER, delay_SC); // nezpusobi pulze
                    timer_start(TGROUP, TNUMBER);
                    timer_state = false;
                } 
            }
        }
    }
}

void app_main(void)
{
//GPIO
    gpio_config_t gpio_out = {};
        gpio_out.intr_type = GPIO_INTR_DISABLE;
        gpio_out.mode = GPIO_MODE_OUTPUT;
        gpio_out.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
        gpio_out.pull_down_en = 0;
        gpio_out.pull_up_en = 0;
    gpio_config(&gpio_out);

    gpio_config_t gpio_in_sens = {};
        gpio_in_sens.intr_type = GPIO_INTR_POSEDGE;
        gpio_in_sens.mode = GPIO_MODE_INPUT;
        gpio_in_sens.pin_bit_mask = GPIO_INPUT_POSEDGE_SEL;
        gpio_in_sens.pull_down_en = 1;
        gpio_in_sens.pull_up_en = 0;    
    gpio_config(&gpio_in_sens);

        gpio_config_t gpio_in_pullup = {};
        gpio_in_pullup.intr_type = GPIO_INTR_ANYEDGE;
        gpio_in_pullup.mode = GPIO_MODE_INPUT;
        gpio_in_pullup.pin_bit_mask = GPIO_INPUT_PULLUP_SEL;
        gpio_in_pullup.pull_down_en = 0;
        gpio_in_pullup.pull_up_en = 1;    
    gpio_config(&gpio_in_pullup);

    
    mode_check(NULL);
    
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(GPIO_INPUT_SENS_1 , gpio_isr_handler, (void*) GPIO_INPUT_SENS_1);
    gpio_isr_handler_add(GPIO_INPUT_SENS_2 , gpio_isr_handler, (void*) GPIO_INPUT_SENS_2);
    gpio_isr_handler_add(GPIO_INPUT_BUTTON , gpio_isr_handler, (void*) GPIO_INPUT_BUTTON);
    gpio_isr_handler_add(GPIO_INPUT_MODE_BUTTON , mode_check, (void*) GPIO_INPUT_MODE_BUTTON);
    gpio_isr_handler_add(GPIO_INPUT_MODE_AUTO , mode_check, (void*) GPIO_INPUT_MODE_AUTO);

    gpio_set_level(GPIO_OUTPUT_MOS, 0);
    gpio_set_level(GPIO_OUTPUT_LED_GR, 1);

    example_tg_timer_init(TGROUP, TNUMBER, true, 1);

//ADC
    ADC_init(ADC_1);
    ADC_init(ADC_2);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

//WIFI
    wifi_init_softap();

    initi_web_page_buffer();
    
    start_webserver();

//OLED
    oled_init();
    
    oled_drawloading();
 
    while(1) {
        vTaskDelay(100 / portTICK_RATE_MS);

        ADC_measure_temperature();

        speed_reset_counter++;
        if (speed_reset_counter == 25)
        {
            speed_reset_counter = 0;
            if (speed_SC == prev_speed)
            {
                speed_SC = 0;
            } else 
            {
                prev_speed = speed_SC;
            }
        }

        oled_loading_progress = oled_drawstats(mode, oled_loading_progress);
    }
}