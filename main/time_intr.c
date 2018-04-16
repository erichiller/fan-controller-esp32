#include <stdio.h>
#include "esp_types.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "soc/timer_group_struct.h"

#include "driver/periph_ctrl.h"
#include "driver/timer.h"

#include "config.h"
#include "time_intr.h"


#define LOGT LOG_TAG_INTR

#define TIMER_SCALE 80									 //  Hardware timer clock divider
#define TIMER_SCALED ( TIMER_BASE_CLK / TIMER_SCALE )	 // 80 million / 80 = 1,000,000

xQueueHandle timer_queue;


/*
 * A simple helper function to print the raw timer counter value
 * and the counter value converted to seconds
 */
void print_timer_counter( uint64_t counter_value ) {
	printf( "Counter: 0x%08x%08x\n", ( uint32_t )( counter_value >> 32 ), ( uint32_t )( counter_value ) );
	printf( "Time   : %.8f µs\n", (double)counter_value / TIMER_SCALE );
}

void log_intr_init(){
	esp_log_level_set( LOG_TAG_INTR, LOG_TAG_INTR_LEVEL );
}



/*
 * Timer group0 ISR handler
 *
 * Note:
 * We don't call the timer API here because they are not declared with IRAM_ATTR.
 * If we're okay with the timer irq not being serviced while SPI flash cache is disabled,
 * we can allocate this interrupt without the ESP_INTR_FLAG_IRAM flag and use the normal API.
 */
void IRAM_ATTR timer_group0_isr( void *para ) {
	int timer_idx = (int)para;

	/* Retrieve the interrupt status and the counter value
       from the timer that reported the interrupt */
	uint32_t intr_status			   = TIMERG0.int_st_timers.val;
	TIMERG0.hw_timer[timer_idx].update = 1;

    uint64_t timer_counter_value = 
        ((uint64_t) TIMERG0.hw_timer[timer_idx].cnt_high) << 32
        | TIMERG0.hw_timer[timer_idx].cnt_low;

	/* Prepare basic event data
       that will be then sent back to the main program task */
	timer_event_t evt;
	evt.timer_group = (timer_group_t)0;
	evt.timer_idx   = (timer_idx_t)timer_idx;
    evt.timer_counter_value = timer_counter_value;

	/* Clear the interrupt
       and update the alarm time for the timer with without reload */
	if( ( intr_status & BIT( timer_idx ) ) && timer_idx == TIMER_0 ) {
		TIMERG0.int_clr_timers.t0 = 1;

	// } else if( ( intr_status & BIT( timer_idx ) ) && timer_idx == TIMER_1 ) {
	// 	TIMERG0.int_clr_timers.t1 = 1;
	} else {
		// not supported event type
	}

	/* After the alarm has been triggered
      we need enable it again, so it is triggered the next time */
	TIMERG0.hw_timer[timer_idx].config.alarm_en = TIMER_ALARM_EN;

	/* Now just send the event data back to the main program task */
	xQueueSendFromISR( timer_queue, &evt, NULL );
}





void timer_setup( int queue_size ) {
	ESP_LOGI( LOGT, "timer_setup" );
	/** timer init **/
	timer_queue = xQueueCreate( queue_size, sizeof( timer_event_t ) );
}

/*
 * Initialize selected timer of the timer group 0
 *
 * timer_idx - the timer number to initialize
 * auto_reload - should the timer auto reload on alarm?
 * timer_interval_microseconds - the interval of alarm to set
 */
void timer_init_group_0( timer_idx_t timer_idx,
								bool		auto_reload,
								double		timer_interval_microseconds ) {
	/* Select and initialize basic parameters of the timer */
	timer_config_t config;
	config.divider	 = TIMER_SCALE;
	config.counter_dir = TIMER_COUNT_UP;
	config.counter_en  = TIMER_PAUSE;
	config.alarm_en	= TIMER_ALARM_DIS;
	config.intr_type   = TIMER_INTR_LEVEL;
	config.auto_reload = auto_reload;
	ESP_LOGI( LOGT, "timer_init_group_0:\n\t divider:\t%i\n\t counter_dir:\t%i\n\t counter_en:\t%i\n\t alarm_en:\t%i\n\t intr_type:\t%i\n\t auto_reload:\t%i\n", config.divider, config.counter_dir, config.counter_en, config.alarm_en, config.intr_type, config.auto_reload );
	ESP_LOGI( LOGT, "timer_init_group_0:\n\t timer_idx:\t%i\n\t timer_interval_microseconds:\t%f\n\t auto_reload:\t%i\n", timer_idx, timer_interval_microseconds, auto_reload );
	timer_init( TIMER_GROUP_0, timer_idx, &config );

	/* Timer's counter will initially start from value below.
       Also, if auto_reload is set, this value will be automatically reload on alarm */
	timer_set_counter_value( TIMER_GROUP_0, timer_idx, 0x00000000ULL );

	/* Configure the alarm value and the interrupt on alarm. */
	timer_set_alarm_value( TIMER_GROUP_0, timer_idx, (uint64_t)timer_interval_microseconds );
	timer_enable_intr( TIMER_GROUP_0, timer_idx );
	timer_isr_register( TIMER_GROUP_0, timer_idx, timer_group0_isr, (void *)timer_idx, ESP_INTR_FLAG_IRAM, NULL );
}




#	undef LOGT