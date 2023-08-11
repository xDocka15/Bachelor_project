
// GPIO
#define GPIO_OUTPUT_MOS       13 
#define GPIO_OUTPUT_LED_GR    12
#define GPIO_OUTPUT_LED_RED   14 
#define GPIO_OUTPUT_TEST       0 
#define GPIO_OUTPUT_FAN       25  
#define GPIO_INPUT_SENS       26 
#define GPIO_INPUT_BUTTON       2 // push button input pin 
#define GPIO_INPUT_MODE_BUTTON  4 // button mode selection pin
#define GPIO_INPUT_MODE_AUTO   16 // automatic mode selection pin
#define ESP_INTR_FLAG_DEFAULT   0 
#define GPIO_INPUT_POSEDGE_SEL  ((1ULL<<GPIO_INPUT_BUTTON))
#define GPIO_INPUT_ANYEDGE_SEL  ((1ULL<<GPIO_INPUT_SENS) | (1ULL<<GPIO_INPUT_MODE_BUTTON) | (1ULL<<GPIO_INPUT_MODE_AUTO))
//#define GPIO_INPUT_DISABLE_SEL  ((1ULL<<GPIO_INPUT_MODE_BUTTON) | (1ULL<<GPIO_INPUT_MODE_AUTO))
#define GPIO_OUTPUT_PIN_SEL     ((1ULL<<GPIO_OUTPUT_MOS) | (1ULL<<GPIO_OUTPUT_LED_GR) | (1ULL<<GPIO_OUTPUT_LED_RED) | (1ULL<<GPIO_OUTPUT_TEST) | (1ULL<<GPIO_OUTPUT_FAN))

int adc_max_coil_voltage = 2000;
int TERM_TEMP_1 = 0;
int TERM_TEMP_2 = 0;


// timers
#define TGROUP (TIMER_GROUP_0)
#define TNUMBER (TIMER_0)
#define TIMER_DIVIDER         (16)  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds
#define coiluptime              0.020 // coil up time in seconds
#define BUTTON_DELAY            0.2 // delay after coil is turned on

volatile bool TIMER_STATE = false;
volatile bool LEDTEST = false;
volatile bool TIMER_DELAY_STATE = false; //Indicates if delay after coil was turned on is counting
volatile bool TIMER_READY = true;
volatile bool TEMP_OVERLOAD = false;
volatile bool SPEED_PRINT = false;
volatile bool PRINT_TEMP = false;
volatile bool INT_TEST = false;
int INT_TEST_COUNTER = 0;

//ADC
#define DEFAULT_VREF            1100        //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES           64          //Multisampling
#define TERM_BETA               3960.00
#define TERM_T0                 25.00
#define TERM_FAN_UP             35.00 // FAN ON above
#define TERM_FAN_DOWN           32.00 // FAN OFF below
#define TERM_CRIT               45 // COIL OFF above
#define TERM_NONCRIT            40 // COIL ON below
#define TERM_R0                 100000.00
#define ADC_1                   (ADC_CHANNEL_6)
#define ADC_2                   (ADC_CHANNEL_4)//7

static esp_adc_cal_characteristics_t *adc_chars;
    static const adc_bits_width_t width = ADC_WIDTH_BIT_12;
    static const adc_atten_t atten = ADC_ATTEN_DB_11;
    static const adc_unit_t unit = ADC_UNIT_1; 

bool ADC_SEL = true;
uint32_t adc_1_reading = 0;
uint32_t TERM_1_V = 0;
uint32_t TERM_2_V = 0;
int adc_interval = 0;

// 
uint64_t timeinsens_SC;
volatile uint64_t speed_SC = 0;
volatile uint64_t speed2_SC = 0;
volatile uint64_t delay_SC = 0;
volatile uint64_t delay2_SC = 0;
volatile int64_t dist_SC = 0;
volatile uint64_t speed_avg_SC = 0;