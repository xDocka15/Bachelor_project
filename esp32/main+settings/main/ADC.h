#include "driver/adc.h"
#include "esp_adc_cal.h"


#define DEFAULT_VREF            1100        //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES           64          //Multisampling
#define TERM_BETA               3960.00
#define TERM_T0                 25.00
#define TERM_FAN_UP             35.00 // FAN ON above also in index.html
#define TERM_FAN_DOWN           32.00 // FAN OFF below also in index.html
#define TERM_CRIT               45.00 // COIL OFF above also in index.html
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
volatile bool TEMP_OVERLOAD;

void ADC_init(int channel)
{
    adc1_config_width(width);
    adc1_config_channel_atten(channel, atten);
    esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, adc_chars);
}

void ADC_measure_temperature(void){
    adc_interval ++;
    if (adc_interval == 20) 
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
        // if (PRINT_TEMP){
        //     printf(" COIL TEMP %d \t TERM_2 %d \n",TERM_TEMP_1,TERM_TEMP_2);
        // }
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