#pragma once

#define LED_GPIO_PIN 12 
#define INDEX_HTML_PATH "/spiffs/index.html"
char index_html[0x1000];
extern volatile int number;
extern volatile int led_state;
extern enum operation mode;

void wifi_init_softap(void);
void initi_web_page_buffer(void);
void start_webserver(void);