#ifndef TIMER_H
#define TIMER_H

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

#define TIMER_SCALE 80									 //  Hardware timer clock divider
#define TIMER_SCALED ( TIMER_BASE_CLK / TIMER_SCALE )	 // 80 million / 80 = 1,000,000


/*
 * Timer Event
 *
 * structure to pass events
 * from the timer interrupt handler to the main program.
 */
typedef struct
{
	timer_group_t timer_group;	// enum , int
	timer_idx_t   timer_idx;	  // enum , int
	uint64_t	  timer_counter_value;
} timer_event_t;


// struct for timer to monitor frequency
extern timer_event_t timer_hz;

extern xQueueHandle timer_queue;

/*
 * A simple helper function to print the raw timer counter value
 * and the counter value converted to seconds
 */
void print_timer_counter( uint64_t counter_value );

void log_intr_init();



/*
 * Timer group0 ISR handler
 *
 * Note:
 * We don't call the timer API here because they are not declared with IRAM_ATTR.
 * If we're okay with the timer irq not being serviced while SPI flash cache is disabled,
 * we can allocate this interrupt without the ESP_INTR_FLAG_IRAM flag and use the normal API.
 */
void IRAM_ATTR timer_group0_isr( void *para );





void timer_setup( int queue_size );

/*
 * Initialize selected timer of the timer group 0
 *
 * timer_idx - the timer number to initialize
 * auto_reload - should the timer auto reload on alarm?
 * timer_interval_microseconds - the interval of alarm to set
 */
void timer_init_group_0( timer_idx_t timer_idx,
								bool		auto_reload,
								double		timer_interval_microseconds );




#endif