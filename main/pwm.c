#define LOGT LOG_TAG_PWM
#include <driver/ledc.h>
#include <inttypes.h>
#include <esp_log.h>

#include "config.h"
#include "pwm.h"

#define DUTY_RESOLUTION LEDC_TIMER_11_BIT    // 11 bits gives me 0-2047 levels of resolution
#define PWM_FREQUENCY 25000


const int PWM_PIN = 19;
uint32_t  PWM_PERCENT = 50;
uint32_t  DUTY_MAX;
ledc_timer_config_t   ledc_timer = {};
ledc_channel_config_t ledc_channel;


void set_duty_percent( int percent ) {
	ESP_LOGD( LOGT, "set_duty_percent(%i) start", percent );

	// returns duty in fractional
	static int MAX = {100};
	static int MIN = {20};

	// Clamp the value
	percent = ( percent >= MIN ) ? percent : MIN;
	percent = ( percent <= MAX ) ? percent : MAX;

	// Compute the controlling values.
	uint32_t duty = ( uint32_t )( ( (double)percent / 100 ) * (double)DUTY_MAX );

	ESP_LOGD( LOGT, "Setting Duty Cycle: %i%% %i/%i;", percent, duty, DUTY_MAX );

	ledc_set_duty( ledc_channel.speed_mode, ledc_channel.channel, duty );
	ledc_update_duty( ledc_channel.speed_mode, ledc_channel.channel );
	PWM_PERCENT = percent;
	ESP_LOGD( LOGT, "set_duty_percent(%i) end", percent );
}    // set_duty_percent()




void set_ledc_channel_config( ) {
	DUTY_MAX     = pow( 2, (double)DUTY_RESOLUTION );
	ledc_channel = ( ledc_channel_config_t ){
	    .gpio_num   = PWM_PIN,
	    .speed_mode = LEDC_HIGH_SPEED_MODE,
	    .channel    = LEDC_CHANNEL_0,
	    .intr_type  = LEDC_INTR_DISABLE,
	    .timer_sel  = LEDC_TIMER_0,
	    .duty       = ( uint32_t )( ( (double)PWM_PERCENT / 100 ) * (double)DUTY_MAX )

	};

	ledc_timer.duty_resolution = LEDC_TIMER_11_BIT;       // resolution of PWM duty
	ledc_timer.freq_hz         = PWM_FREQUENCY;           // frequency of PWM signal
	ledc_timer.speed_mode      = LEDC_HIGH_SPEED_MODE;    // timer mode
	ledc_timer.timer_num       = LEDC_TIMER_0;            // timer index
	ESP_LOGD( LOGT, "\n:LEDC INIT VALUES\nledc_timer.duty_resolution:\t%i\nledc_channel.freq_hz:\t%i\nledc_channel.duty:\t%i\nDUTY_MAX:\t%i\n", ledc_timer.duty_resolution, ledc_timer.freq_hz, ledc_channel.duty, DUTY_MAX );

	ledc_timer_config( &ledc_timer );
	ledc_channel_config( &ledc_channel );
}




#undef LOGT