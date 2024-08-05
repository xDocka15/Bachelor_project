#include "ADC.h"
#include "main.h"

esp_adc_cal_characteristics_t *adc_chars;
 const adc_bits_width_t width = ADC_WIDTH_BIT_12;
 const adc_atten_t atten = ADC_ATTEN_DB_11;
 const adc_unit_t unit = ADC_UNIT_1;

void ADC_init(int channel)
{
    adc1_config_width(width);
    adc1_config_channel_atten(channel, atten);
    esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, adc_chars);
}

void ADC_measure_temperature(void)
{
    adc_interval ++;
    if (adc_interval == 20) 
    {
        adc_interval = 0;
        if (adc_sel)
        {
            adc_1_reading = adc1_get_raw((adc1_channel_t)ADC_1);
            term_1_v = esp_adc_cal_raw_to_voltage(adc_1_reading, adc_chars);
            int TERM_1_R1 = term_1_v/1000.00*56000.00/(5-term_1_v/1000.00);
            term_temp_1 = 1/((1/(TERM_T0+273))+(1/TERM_BETA)*log(TERM_1_R1/TERM_R0))-273;
            adc_sel = false;
        } else 
        {
            adc_1_reading = adc1_get_raw((adc1_channel_t)ADC_2);
            term_2_v = esp_adc_cal_raw_to_voltage(adc_1_reading, adc_chars);
            int TERM_2_R1 = term_2_v/1000.00*56000.00/(5-term_2_v/1000.00);
            term_temp_2 = 1/((1/(TERM_T0+273))+(1/TERM_BETA)*log(TERM_2_R1/TERM_R0))-273;
            adc_sel = true;
        }
        if (term_temp_2 > TERM_FAN_UP)
        {
            gpio_set_level(GPIO_OUTPUT_FAN, 1);            
        } else if (term_temp_2 < TERM_FAN_DOWN) 
        {
            gpio_set_level(GPIO_OUTPUT_FAN, 0);
        }
        if (term_temp_1 > TERM_CRIT || term_temp_2 > TERM_CRIT)
        {
            temp_overload = true;
            gpio_set_level(GPIO_OUTPUT_LED_RED, 1);
            gpio_set_level(GPIO_OUTPUT_LED_GR, 0);
        } else if (term_temp_1 < TERM_NONCRIT && term_temp_2 < TERM_NONCRIT) 
        {
            temp_overload = false;
            gpio_set_level(GPIO_OUTPUT_LED_RED, 0);
            gpio_set_level(GPIO_OUTPUT_LED_GR, 1);
        }
    }
}