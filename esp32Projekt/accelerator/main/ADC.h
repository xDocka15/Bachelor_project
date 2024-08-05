#pragma once

#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "main.h"


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
#define ADC_2                   (ADC_CHANNEL_4)


bool adc_sel;
extern int term_temp_1;
extern int term_temp_2;
extern uint32_t adc_1_reading;
extern uint32_t term_1_v;
extern uint32_t term_2_v;
extern int adc_interval;
volatile bool temp_overload;

void ADC_init(int channel);
void ADC_measure_temperature(void);
