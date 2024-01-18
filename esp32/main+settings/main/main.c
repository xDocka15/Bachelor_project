#include "main_def.h"

// sepnuti vetraku 500 us pulse na sensoru - reseni v preruseni

//**************************************************************
int adc_max_coil_voltage = 2000;
int TERM_TEMP_1 = 0;
int TERM_TEMP_2 = 0;

volatile bool TIMER_STATE = false;
volatile bool LEDTEST = false;
volatile bool TIMER_DELAY_STATE = false; //Indicates if delay after coil was turned on is counting
volatile bool TIMER_READY = true;
volatile bool TEMP_OVERLOAD = false;
volatile bool SPEED_PRINT = false;  //set
volatile bool PRINT_TEMP = false;   //set
volatile bool INT_TEST = false;     //set
int INT_TEST_COUNTER = 0;

bool ADC_SEL = true;
uint32_t adc_1_reading = 0;
uint32_t TERM_1_V = 0;
uint32_t TERM_2_V = 0;
int adc_interval = 0;

volatile uint64_t bdiam_SC = 0.130 * TIMER_SCALE; // 29 calculated 27 // new distance of sensors
volatile uint64_t dist_to_coil_SC = 0.120 * TIMER_SCALE; // distance to the end of pulse
uint64_t timeinsens_SC;
volatile uint64_t speed_SC = 0;
volatile uint64_t speed2_SC = 0;
volatile uint64_t delay_SC = 0;
volatile uint64_t delay2_SC = 0;
volatile int64_t dist_SC = 0;
volatile uint64_t speed_avg_SC = 0;
double speed_max = 0.01;

double raw = 0;
double speed = 0;
double speed2 = 0;
double delay2 = 0;
double delay = 0;
double speed_avg = 0;

 volatile bool SENS_SEQUENCE = true; // set
 volatile bool EN_BUTTON = false;
//**************************************************************

static const char *TAG = "main";

static void IRAM_ATTR mode_check(void)
{
    //SPEED_PRINT = true;
    if (gpio_get_level(GPIO_INPUT_MODE_BUTTON) == 0)  //bug bez tohoto vyvola tlacitko deleni nulou pri MODE_AUTO
    {
        gpio_intr_disable(GPIO_INPUT_BUTTON);
    } else {
        gpio_intr_enable(GPIO_INPUT_BUTTON);
    }

    if (gpio_get_level(GPIO_INPUT_MODE_AUTO) == 0)  //bug reseni: sepnuti civky oklame sensor v MODE_BUTTON
    {
        //gpio_intr_disable(GPIO_INPUT_SENS_1);
        //gpio_intr_disable(GPIO_INPUT_SENS_2);
    } else {
        gpio_intr_enable(GPIO_INPUT_SENS_1);
        gpio_intr_enable(GPIO_INPUT_SENS_2);
    }
    if (gpio_get_level(GPIO_INPUT_MODE_WIFI) == 1){
        EN_BUTTON = true;
    } else {
        EN_BUTTON = false;
    }
}

static bool IRAM_ATTR timer_1_isr_callback(void *args)
{
    BaseType_t high_task_awoken = pdFALSE;

    if (TIMER_STATE && !TIMER_DELAY_STATE)
    { // output pin to high and set next callback in "coiluptime" time
        gpio_set_level(GPIO_OUTPUT_MOS, 1);
        timer_pause(TGROUP, TNUMBER);
        timer_set_counter_value(TGROUP, TNUMBER, 0);
        timer_group_set_alarm_value_in_isr(TGROUP, TNUMBER, coiluptime * TIMER_SCALE);
        timer_start(TGROUP, TNUMBER);
        TIMER_STATE = false;
        LEDTEST = true;
        gpio_set_level(GPIO_OUTPUT_LED_GR, 0); // new_led_test 2/2

    } else if (!TIMER_STATE && !TIMER_DELAY_STATE)
    { // output to low and enable delay
        TIMER_DELAY_STATE = true;
        gpio_set_level(GPIO_OUTPUT_MOS, 0);
        timer_pause(TGROUP, TNUMBER);
        timer_set_counter_value(TGROUP, TNUMBER, 0);
        timer_group_set_alarm_value_in_isr(TGROUP, TNUMBER, BUTTON_DELAY * TIMER_SCALE);
        timer_start(TGROUP, TNUMBER);
    } else if (TIMER_DELAY_STATE) 
    { //set next callback to some long duration better way would be disable interrupt
        TIMER_DELAY_STATE = false;
        TIMER_READY = true;
        timer_pause(TGROUP, TNUMBER);
        timer_set_counter_value(TGROUP, TNUMBER, 0);
        timer_group_set_alarm_value_in_isr(TGROUP, TNUMBER, 10 * TIMER_SCALE);
        //gpio_intr_enable(GPIO_INPUT_SENS_1 );  //bug pokus o reseni druheho pulzu na sensoru_zaroven_s civkou 2/2
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

void IRAM_ATTR gpio_isr_handler(void* arg)
{
    if (!TIMER_DELAY_STATE && !TEMP_OVERLOAD && TIMER_READY) 
    {
        if ((gpio_get_level(GPIO_INPUT_BUTTON) == 1 && gpio_get_level(GPIO_INPUT_MODE_BUTTON) == 1) || (gpio_get_level(GPIO_INPUT_MODE_WIFI) == 1 && gpio_get_level(GPIO_INPUT_MODE_BUTTON) == 0))
        { 
            TIMER_STATE = true;
            TIMER_READY = false;
            LEDTEST = true;
            //INT_TEST = true;
            timer_pause(TGROUP, TNUMBER);
            timer_set_counter_value(TGROUP, TNUMBER, 0);
            timer_set_alarm_value(TGROUP, TNUMBER, 0.005 * TIMER_SCALE);
            timer_start(TGROUP, TNUMBER);
        }
        if ((gpio_get_level(GPIO_INPUT_SENS_1 ) == 1)) 
        { // (kulicka prisla do sensoru) reset timeru pro mereni delky zakryti senzoru
            SENS_SEQUENCE = false;
            timer_pause(TGROUP, TNUMBER);
            timer_set_counter_value(TGROUP, TNUMBER, 0);
            timer_set_alarm_value(TGROUP, TNUMBER, 10 * TIMER_SCALE); // bug pokud je senzor zakryt dele >> vyvolani deleni nulou
            timer_start(TGROUP, TNUMBER);
        } else if ((gpio_get_level(GPIO_INPUT_SENS_2 ) == 1) && !SENS_SEQUENCE) { // kulicka opustila sensor
            SENS_SEQUENCE = true;
            SPEED_PRINT = true;
            delay_SC = 0; 
            delay2_SC = 0;
            timer_pause(TGROUP, TNUMBER);
            timer_get_counter_value(TGROUP, TNUMBER, &timeinsens_SC);
            if (timeinsens_SC != 0)
            {
                speed_SC = TIMER_SCALE * bdiam_SC / timeinsens_SC;
                dist_SC = log((speed_SC*1.000/TIMER_SCALE)/1.632)/(-0.5) * TIMER_SCALE + dist_to_coil_SC;
                speed2_SC = 1.632*exp(-0.5 * dist_SC/TIMER_SCALE) * TIMER_SCALE;
                speed_avg_SC = (speed_SC + speed2_SC)/2;
                delay_SC = TIMER_SCALE * dist_to_coil_SC / speed_avg_SC;
                if ((delay_SC > coiluptime * TIMER_SCALE) && gpio_get_level(GPIO_INPUT_MODE_AUTO) == 1) // reseni bug vetrak spina sensor + omezeni max rychlosti
                {
                    TIMER_STATE = true;
                    TIMER_READY = false;
                    delay2_SC = delay_SC - (coiluptime * TIMER_SCALE);// + (coillength_SC * TIMER_SCALE /speed_SC);
                    timer_set_counter_value(TGROUP, TNUMBER, 0);              
                    timer_set_alarm_value(TGROUP, TNUMBER, delay2_SC);
                    timer_start(TGROUP, TNUMBER);
                    gpio_set_level(GPIO_OUTPUT_LED_GR, 1); // new_led_test 1/2
                } else {
                    INT_TEST = true;
                    timer_start(TGROUP, TNUMBER);
                } 
            }
            //gpio_intr_disable(GPIO_INPUT_SENS_1 ); // bug pokus o reseni druheho pulzu na sensoru_zaroven_s civkou 1/2
        }
    }
}

void app_main(void)
{
//GPIO
    gpio_config_t io_conf = {};
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
        io_conf.pull_down_en = 0;
        io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
        io_conf.intr_type = GPIO_INTR_POSEDGE;
        io_conf.pin_bit_mask = GPIO_INPUT_POSEDGE_SEL;
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pull_down_en = 1;
        io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    // io_conf.intr_type = GPIO_INTR_DISABLE;
    // io_conf.pin_bit_mask = GPIO_INPUT_DISABLE_SEL;
    // gpio_config(&io_conf);
    
    mode_check();
    
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(GPIO_INPUT_SENS_1 , gpio_isr_handler, NULL);
    gpio_isr_handler_add(GPIO_INPUT_SENS_2 , gpio_isr_handler, NULL);
    gpio_isr_handler_add(GPIO_INPUT_BUTTON , gpio_isr_handler, NULL);
    gpio_isr_handler_add(GPIO_INPUT_MODE_BUTTON , mode_check, NULL);
    gpio_isr_handler_add(GPIO_INPUT_MODE_AUTO , mode_check, NULL);
    gpio_isr_handler_add(GPIO_INPUT_MODE_WIFI , mode_check, NULL);

    gpio_set_level(GPIO_OUTPUT_MOS, 0);
    gpio_set_level(GPIO_OUTPUT_LED_GR, 0);
    gpio_set_level(GPIO_OUTPUT_TEST, 0);

    example_tg_timer_init(TGROUP, TNUMBER, true, 1);

//ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    ADC_init(ADC_1);
    ADC_init(ADC_2);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    wifi_init_softap();
    initi_web_page_buffer();
    start_webserver();
    
    while(1) {
        vTaskDelay(100 / portTICK_RATE_MS);

        if (INT_TEST)
        {
            INT_TEST = false;
            INT_TEST_COUNTER ++;
            printf("INT_TEST %d\n", INT_TEST_COUNTER);
        }

        if (gpio_get_level(GPIO_INPUT_BUTTON) == 1)
        {
            gpio_set_level(GPIO_OUTPUT_FAN, 1);            
        }

        if (SPEED_PRINT)
        {
            SPEED_PRINT = false;
            raw = timeinsens_SC;
            speed = speed_SC;
            speed2 = speed2_SC;
            delay2 = delay2_SC;
            delay = delay_SC;
            speed_avg = speed_avg_SC;
            if (speed > speed_max)
            {
                speed_max = speed;
            }
            printf("raw %lld \t time %f \t speed %f \t speed2 %f \t avg speed %f \t delay %f \t delay2 %f \t dist2 %f\n",timeinsens_SC ,raw/TIMER_SCALE, speed/TIMER_SCALE, speed2/TIMER_SCALE, speed_avg/TIMER_SCALE , delay/TIMER_SCALE, delay2/TIMER_SCALE, (speed/TIMER_SCALE)*(delay2/TIMER_SCALE));   
        }

        if (LEDTEST)
        {
            LEDTEST = false;
            gpio_set_level(GPIO_OUTPUT_LED_GR, 1);
            vTaskDelay(250 / portTICK_RATE_MS);
            gpio_set_level(GPIO_OUTPUT_LED_GR, 0);
        }

        adc_interval ++;
        if (adc_interval == 25) 
        {
            adc_interval = 0;
            if (ADC_SEL)
            {
                adc_1_reading = adc1_get_raw((adc1_channel_t)ADC_1);
                TERM_1_V = esp_adc_cal_raw_to_voltage(adc_1_reading, adc_chars);
                int TERM_1_R1 = TERM_1_V/1000.00*56000.00/(5-TERM_1_V/1000.00);
                TERM_TEMP_1 = 1/((1/(TERM_T0+273))+(1/TERM_BETA)*log(TERM_1_R1/TERM_R0))-273;
                ADC_SEL = false;
            } else {
                adc_1_reading = adc1_get_raw((adc1_channel_t)ADC_2);
                TERM_2_V = esp_adc_cal_raw_to_voltage(adc_1_reading, adc_chars);
                int TERM_2_R1 = TERM_2_V/1000.00*56000.00/(5-TERM_2_V/1000.00);
                TERM_TEMP_2 = 1/((1/(TERM_T0+273))+(1/TERM_BETA)*log(TERM_2_R1/TERM_R0))-273;
                ADC_SEL = true;
            }
            if (PRINT_TEMP){
                printf(" COIL TEMP %d \t TERM_2 %d \n",TERM_TEMP_1,TERM_TEMP_2);
            }
            if (TERM_TEMP_1 > TERM_FAN_UP || TERM_TEMP_2 > TERM_FAN_UP)
            {
                //ESP_LOGI(TAG, "fan up");
                gpio_set_level(GPIO_OUTPUT_FAN, 1);            
            } else if (TERM_TEMP_1 < TERM_FAN_DOWN && TERM_TEMP_2 < TERM_FAN_DOWN) 
            {
                //ESP_LOGI(TAG, "fan down");
                gpio_set_level(GPIO_OUTPUT_FAN, 0);
            }
            if (TERM_TEMP_1 > TERM_CRIT || TERM_TEMP_2 > TERM_CRIT)
            {
                //ESP_LOGI(TAG, "coil down");
                TEMP_OVERLOAD = true;
                gpio_set_level(GPIO_OUTPUT_LED_RED, 1);
            } else if (TERM_TEMP_1 < TERM_NONCRIT && TERM_TEMP_2 < TERM_NONCRIT) 
            {
                //ESP_LOGI(TAG, "coil up");
                TEMP_OVERLOAD = false;
                gpio_set_level(GPIO_OUTPUT_LED_RED, 0);
            }
        }

    }
}