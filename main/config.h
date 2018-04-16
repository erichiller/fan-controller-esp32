#ifndef CONFIG
#define CONFIG

#include "esp_log.h"
#include "driver/timer.h"

#define LOG_TAG_INTR "timer_interrupts"
#define LOG_TAG_INTR_LEVEL ESP_LOG_INFO
#define LOG_u8g2_hal_LEVEL ESP_LOG_INFO
#define LOG_TAG_TEMP_SENSOR "temp_sensor"
#define LOG_TAG_TEMP_SENSOR_LEVEL ESP_LOG_WARN
#define LOG_TAG_PWM "pwm"
#define LOG_TAG_PWM_LEVEL ESP_LOG_INFO
#define LOG_TAG_WEB_SERVER "WEB_SERVER"
#define LOG_TAG_WEB_SERVER_LEVEL ESP_LOG_VERBOSE
#define LOG_TAG_UI "UI_SCREEN"
#define LOG_TAG_UI_LEVEL ESP_LOG_VERBOSE



void inline static config_log_levels(){
	esp_log_level_set( "*", ESP_LOG_DEBUG );
	esp_log_level_set( "RTC_MODULE", ESP_LOG_INFO );
	esp_log_level_set(LOG_TAG_TEMP_SENSOR, LOG_TAG_TEMP_SENSOR_LEVEL);
	esp_log_level_set(LOG_TAG_PWM, LOG_TAG_PWM_LEVEL);
	esp_log_level_set(LOG_TAG_WEB_SERVER, LOG_TAG_WEB_SERVER_LEVEL);
	esp_log_level_set(LOG_TAG_UI, LOG_TAG_UI_LEVEL);
}

/** WebServer **/
#define WebServerTask_STACK_WORDS 8192
#define WebServerTask_NAME "WebServerTask"
#define WebServerTask_PRIORITY 2


/* wifi max wait time (miliseconds) */
#define WIFI_MAX_WAIT 15000

/**
 *  timer 
 **/
#define TIMER_HZ_GROUP TIMER_GROUP_0
#define TIMER_HZ_IDX TIMER_0


/*****
 * Log Colors
 * #define LOG_COLOR_BLACK   "30"
 * #define LOG_COLOR_RED     "31"
 * #define LOG_COLOR_GREEN   "32"
 * #define LOG_COLOR_BROWN   "33"
 * #define LOG_COLOR_BLUE    "34"
 * #define LOG_COLOR_PURPLE  "35"
 * #define LOG_COLOR_CYAN    "36"
 ***/
// add white for bolding
#define LOG_COLOR_WHITE "37"
// defaults
// #define LOG_COLOR_E       LOG_COLOR(LOG_COLOR_RED)
// #define LOG_COLOR_W       LOG_COLOR(LOG_COLOR_BROWN)
// edits
#undef LOG_COLOR_I
#define LOG_COLOR_I LOG_COLOR( LOG_COLOR_BLUE )
#undef LOG_COLOR_D
#define LOG_COLOR_D LOG_COLOR( LOG_COLOR_GREEN )
#undef LOG_COLOR_V
#define LOG_COLOR_V LOG_BOLD( LOG_COLOR_WHITE )

#endif