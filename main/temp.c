
#include "esp_system.h"
#include <stdio.h>

#include <esp_log.h>
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_event.h"

#include "temp.h"
#include "helper.h"
#include "config.h"
#include "ui.h"

#define LOGT LOG_TAG_TEMP_SENSOR


#define sensor_adc_atten ADC_ATTEN_11db
#define sensor_adc_resolution ADC_WIDTH_BIT_12

int                                   NO_OF_SAMPLES = 64;
adc_unit_t                            ADC_UNIT      = ADC_UNIT_1;
static esp_adc_cal_characteristics_t *adc_chars;


adc1_channel_t sensor_adc_channel = ADC1_GPIO33_CHANNEL;
int            sensorPin          = 33;    // select the input pin for the Sensor :: Pin 35 is ADC1_CHANNEL_7
float          centigrade         = 0;
float          farenheit          = 0;

void init_sensor(){

	/*** ADC ***/
	adc1_config_channel_atten( sensor_adc_channel, sensor_adc_atten );
	adc1_config_width( sensor_adc_resolution );
	ESP_LOGD( LOGT, "ADC configured" );
}

void SensorTask( void *arg ) {
	adc_chars                    = (esp_adc_cal_characteristics_t *)calloc( 1, sizeof( esp_adc_cal_characteristics_t ) );
	esp_adc_cal_value_t val_type = esp_adc_cal_characterize( ADC_UNIT, sensor_adc_atten, sensor_adc_resolution, 1100, adc_chars );
	//Check type of calibration value used to characterize ADC
	ESP_LOGV( LOGT, "ADC CHARACTERISTIC TYPE=%i", (int)val_type );
	while( 1 ) {
		/*****************************************
		 **** Read the value from the sensor: ****
		 *****************************************/
		int milliVolts = measure_adc_multisample_raw( );

		centigrade = ( milliVolts - 500 ) / 10;
		farenheit  = ( centigrade * 9.0 / 5.0 ) + 32.0;

		// ESP_LOGI( LOGT, "%" PRId64 " ms || %i milliVolt || %f degrees C || %f degrees F\n", millis( ), milliVolts, centigrade, farenheit );
		vTaskDelay( pdMS_TO_TICKS( 1000 ) );

		UpdateScreen( centigrade );
	}
}

uint32_t measure_adc_multisample_raw( ) {
	ESP_LOGD( LOGT, "measure_adc_multisample_raw() sampling begin" );


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
	ESP_LOGD( LOGT, "measure_adc_multisample_raw() sampling complete" );

	return voltage;
}

#undef LOGT
