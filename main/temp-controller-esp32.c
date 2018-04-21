#include "esp_system.h"
#include "sdkconfig.h"

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "stdint.h"

#include <esp_log.h>
#include "nvs_flash.h"
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include "driver/adc.h"
#include "esp_adc_cal.h"


#include "helper.h"
#include "net_wifi.h"
#include "config.h"
#include "WebServer.h"
#include "TelnetServer.h"
#include "pwm.h"
#include "ui.h"
#include "temp.h"

// #define commandBuf_MAX 64
#define LOGT "temp-controller-esp32"


void app_main( void ) {
	/* Initialize NVS — it is used to store PHY calibration data */
	esp_err_t ret = nvs_flash_init( );
	if( ret == ESP_ERR_NVS_NO_FREE_PAGES ) {
		ESP_ERROR_CHECK( nvs_flash_erase( ) );
		ret = nvs_flash_init( );
		ESP_LOGI( "FLASH_NVS", "nvs flash had to be init, erased" );
	}
	ESP_ERROR_CHECK( ret );
	/***********************
	 **** SETUP LOGGING ****
	 **********************/
	config_log_levels( );
	ESP_LOGI( "FLASH_NVS", "init.done." );

	ESP_LOGD( LOGT, "notice; debug    log levels are enabled" );
	set_ledc_channel_config( );
	init_display( );

	init_sensor();

	net_wifi_connect( );

	ESP_LOGD( LOGT, "creating main_loop" );

	xTaskCreate( &SensorTask, "SensorTask", 8192, NULL, 1, NULL );
	xTaskCreate( &ProcessMenuInput, "Button_Presses", 4096, NULL, 3, NULL );

	xTaskHandle WebServerTask_handle;
	ret = xTaskCreate( &WebServerTask,
	                   WebServerTask_NAME,
	                   WebServerTask_STACK_WORDS,
	                   NULL,
	                   WebServerTask_PRIORITY,
	                   &WebServerTask_handle );

	if( ret != pdPASS ) {
		ESP_LOGI( LOGT, "create task %s failed", WebServerTask_NAME );
	}
	xTaskHandle TelnetServerTask_handle;
	ret = xTaskCreate( &TelnetServerTask,
	                   TelnetServerTask_NAME,
	                   TelnetServerTask_STACK_WORDS,
	                   NULL,
	                   TelnetServerTask_PRIORITY,
	                   &TelnetServerTask_handle );

	if( ret != pdPASS ) {
		ESP_LOGI( LOGT, "create task %s failed", TelnetServerTask_NAME );
	}
}


#undef LOGT
