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
#include "config.h"
#include "time_intr.h"
#include "WebServer.h"
#include "net_wifi.h"
#include "pwm.h"

// Forward declarations
static void main_loop( void *arg );
uint32_t    measure_adc_multisample_raw( );

// #define commandBuf_MAX 64
#define LOGT "temp-controller-esp32"

#define sensor_adc_atten ADC_ATTEN_11db
#define sensor_adc_resolution ADC_WIDTH_BIT_12
adc1_channel_t sensor_adc_channel = ADC1_GPIO33_CHANNEL;
int            sensorPin          = 33;    // select the input pin for the Sensor :: Pin 35 is ADC1_CHANNEL_7
float          centigrade         = 0;
float          farenheit          = 0;

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
	vTaskDelay( 1 );    // <== Insert delay here

	/*** ADC ***/
	adc1_config_channel_atten( sensor_adc_channel, sensor_adc_atten );
	adc1_config_width( sensor_adc_resolution );
	ESP_LOGD( LOGT, "ADC configured" );

	net_wifi_connect( );

	int period_HZ = 1;    // 1 Hz ;; once per second
	// setup timer for periodic
	timer_event_t timer_hz = {};
	timer_hz.timer_group   = TIMER_HZ_GROUP;
	timer_hz.timer_idx     = TIMER_HZ_IDX;
	ESP_LOGD( LOGT, "beginning timer setup" );
	timer_setup( 10 );
	ESP_LOGD( LOGT, "timer queue size set" );
	ESP_LOGD( LOGT, "\nTIMER_SCALE:\t %i\nTIMER_SCALED:\t %i\nSAMPLE_RATE_HZ:\t %i\n TIMER/SAMPLE: \t %f\n", TIMER_SCALE, TIMER_SCALED, period_HZ, (double)( TIMER_SCALED / period_HZ ) );

	//                  index              reload?     value in microseconds
	timer_init_group_0( timer_hz.timer_idx, true, ( TIMER_SCALED / period_HZ ) );
	ESP_LOGD( LOGT, "creating main_loop" );

	xTaskCreate( &main_loop, "main_loop", 8192, NULL, 1, NULL );

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
}

static void main_loop( void *arg ) {

	while( 1 ) {
		timer_event_t evt;
		// xEventGroupWaitBits( wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY );
		xQueueReceive( timer_queue, &evt, portMAX_DELAY );

		// ESP_LOGI( "timer event loop:%i", (int)evt );


		// ESP_LOGI(LOGT, "Group[%d], timer[%d] alarm event\n", evt.timer_group, evt.timer_idx );

		/* Print the timer values passed by event */
		// printf( "------- EVENT TIME --------\n" );
		// print_timer_counter( evt.timer_counter_value );

		// /* Print the timer values as visible by this task */
		// printf( "-------- TASK TIME --------\n" );
		// uint64_t task_counter_value;
		// timer_get_counter_value( evt.timer_group, evt.timer_idx, &task_counter_value );
		// print_timer_counter( task_counter_value );

		/** demo block **/
		// demo();

		// if( hue_hub_connected != true ) {
		// 	if( hue_mbed_open_dtls( ) ) {
		// 		ESP_LOGI( LOGT, "hue_mbed_open_dtls( ... ) successful ; hue_hub_connected is now TRUE" )
		// 		hue_hub_connected = true;
		// 	}
		// }
		// if( hue_hub_connected == true ) {
		// 	hue_mbed_tx( outcolors[0].red, outcolors[0].green, outcolors[0].blue );
		// }
		/** end demo block **/


		// Parse any pending commands.
		// parserLoop( );

		// ESP_LOGD(LOGT, "end of main_loop, restarting timer");
		// timer_start( TIMER_GROUP_0, TIMER_0 );



		// ESP_LOGD( LOGT , "creating client wifi" );


		/*****************************************
		 **** Read the value from the sensor: ****
		 *****************************************/
		int milliVolts = measure_adc_multisample_raw( );

		centigrade = ( milliVolts - 500 ) / 10;
		farenheit  = ( centigrade * 9.0 / 5.0 ) + 32.0;

		// ESP_LOGI( LOGT, "%" PRId64 " ms || %i milliVolt || %f degrees C || %f degrees F\n", millis( ), milliVolts, centigrade, farenheit );

		UpdateScreen(centigrade);
	}
}

int        NO_OF_SAMPLES = 64;
adc_unit_t ADC_UNIT      = ADC_UNIT_1;


#undef LOGT
#define LOGT LOG_TAG_TEMP_SENSOR
uint32_t measure_adc_multisample_raw( ) {
	ESP_LOGD( LOGT, "measure_adc_multisample_raw() sampling begin" );

	static esp_adc_cal_characteristics_t *adc_chars;

	adc_chars                    = (esp_adc_cal_characteristics_t *)calloc( 1, sizeof( esp_adc_cal_characteristics_t ) );
	esp_adc_cal_value_t val_type = esp_adc_cal_characterize( ADC_UNIT, sensor_adc_atten, sensor_adc_resolution, 1100, adc_chars );
	//Check type of calibration value used to characterize ADC
	ESP_LOGV( LOGT, "ADC CHARACTERISTIC TYPE=%i", (int)val_type );
	//Continuously sample ADC1
	// while( 1 ) {
	uint32_t adc_reading = 0;
	//Multisampling
	for( int i = 0; i < NO_OF_SAMPLES; i++ ) {
		if( ADC_UNIT == ADC_UNIT_1 ) {
			adc_reading += adc1_get_raw( (adc1_channel_t)sensor_adc_channel );
		} else {
			int raw;
			adc2_get_raw( (adc2_channel_t)sensorPin, ADC_WIDTH_BIT_12, &raw );
			adc_reading += raw;
		}
	}
	adc_reading /= NO_OF_SAMPLES;
	//Convert adc_reading to voltage in mV
	uint32_t voltage = esp_adc_cal_raw_to_voltage( adc_reading, adc_chars );
	ESP_LOGI( LOGT, "Raw: %d\tVoltage: %dmV\n", adc_reading, voltage );
	// vTaskDelay( pdMS_TO_TICKS( 1000 ) );
	// }
	ESP_LOGD( LOGT, "measure_adc_multisample_raw() sampling complete" );

	return voltage;
}



#undef LOGT
