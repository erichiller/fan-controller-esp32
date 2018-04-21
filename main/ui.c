
#include "stdint.h"

#include <math.h>
#include <esp_log.h>
#include "u8g2.h"
#include "u8x8.h"

#include "u8g2_esp32_hal.h"

#include "config.h"
#include "helper.h"
#include "net_wifi.h"

#define LOGT LOG_TAG_UI


/**************************************
 ******* SCREEN / DISPLAY / u8g2 ******
 *************************************/
u8g2_t u8g2;

// CLK - GPIO14
#define PIN_CLK 18

// MOSI - GPIO 13
#define PIN_MOSI 23

// CS - GPIO 15
#define PIN_CS 5

// DC - GPIO 27
#define PIN_DC 21

// RESET - GPIO 26
#define PIN_RESET 22



/** Menu
 */

uint8_t MENU_pin_select = 25;    // button. must connect other side to ground
uint8_t MENU_pin_next   = 26;    // button. must connect other side to ground
uint8_t MENU_pin_prev   = 32;    // button. must connect other side to ground
uint8_t MENU_pin_up     = 4;
uint8_t MENU_pin_down   = 12;
uint8_t MENU_pin_home   = 14;



// pinMode( MENU_pin_select, INPUT_PULLUP );
// pinMode( MENU_pin_next, INPUT_PULLUP );
// pinMode( MENU_pin_prev, INPUT_PULLUP );
// pinMode( MENU_pin_up, INPUT_PULLUP );
// pinMode( MENU_pin_down, INPUT_PULLUP );
// pinMode( MENU_pin_home, INPUT_PULLUP );


// u8g2.begin( analogInputToDigitalPin( MENU_pin_select ), analogInputToDigitalPin( MENU_pin_next ), analogInputToDigitalPin( MENU_pin_prev ), analogInputToDigitalPin( MENU_pin_up ), MENU_pin_down, MENU_pin_home );    // https://github.com/olikraus/u8g2/wiki/u8g2reference#begin
// u8g2.begin( MENU_pin_select, MENU_pin_next, MENU_pin_prev, MENU_pin_up, MENU_pin_down, MENU_pin_home );    // https://github.com/olikraus/u8g2/wiki/u8g2reference#begin


// #define u8x8_SetPin( u8x8, pin, val ) ( u8x8 )->pins[pin] = ( val )
// #define u8x8_SetMenuSelectPin( u8x8, val )
// #define u8x8_SetMenuNextPin( u8x8, val ) u8x8_SetPin( ( u8x8 ), U8X8_PIN_MENU_NEXT, ( val ) )
// #define u8x8_SetMenuPrevPin( u8x8, val ) u8x8_SetPin( ( u8x8 ), U8X8_PIN_MENU_PREV, ( val ) )
// #define u8x8_SetMenuHomePin( u8x8, val ) u8x8_SetPin( ( u8x8 ), U8X8_PIN_MENU_HOME, ( val ) )
// #define u8x8_SetMenuUpPin( u8x8, val ) u8x8_SetPin( ( u8x8 ), U8X8_PIN_MENU_UP, ( val ) )
// #define u8x8_SetMenuDownPin( u8x8, val ) u8x8_SetPin( ( u8x8 ), U8X8_PIN_MENU_DOWN, ( val ) )


void display_pins( ) {
	ESP_LOGI( "ui_button", "gpio button U8X8_PIN_MENU_SELECT[%i] pin is %i @ %i ", U8X8_PIN_MENU_SELECT, u8g2_GetU8x8( &u8g2 )->pins[U8X8_PIN_MENU_SELECT], gpio_get_level( u8g2_GetU8x8( &u8g2 )->pins[U8X8_PIN_MENU_SELECT] ) );

	ESP_LOGI( "ui_button", "gpio button U8X8_PIN_MENU_NEXT[%i] pin is %i @ %i ", U8X8_PIN_MENU_NEXT, u8g2_GetU8x8( &u8g2 )->pins[U8X8_PIN_MENU_NEXT], gpio_get_level( u8g2_GetU8x8( &u8g2 )->pins[U8X8_PIN_MENU_NEXT] ) );
	ESP_LOGI( "ui_button", "gpio button U8X8_PIN_MENU_PREV[%i] pin is %i @ %i ", U8X8_PIN_MENU_PREV, u8g2_GetU8x8( &u8g2 )->pins[U8X8_PIN_MENU_PREV], gpio_get_level( u8g2_GetU8x8( &u8g2 )->pins[U8X8_PIN_MENU_PREV] ) );
	ESP_LOGI( "ui_button", "gpio button U8X8_PIN_MENU_UP[%i] pin is %i @ %i ", U8X8_PIN_MENU_UP, u8g2_GetU8x8( &u8g2 )->pins[U8X8_PIN_MENU_UP], gpio_get_level( u8g2_GetU8x8( &u8g2 )->pins[U8X8_PIN_MENU_UP] ) );
	ESP_LOGI( "ui_button", "gpio button U8X8_PIN_MENU_DOWN[%i] pin is %i @ %i ", U8X8_PIN_MENU_DOWN, u8g2_GetU8x8( &u8g2 )->pins[U8X8_PIN_MENU_DOWN], gpio_get_level( u8g2_GetU8x8( &u8g2 )->pins[U8X8_PIN_MENU_DOWN] ) );
	ESP_LOGI( "ui_button", "gpio button U8X8_PIN_MENU_HOME[%i] pin is %i @ %i ", U8X8_PIN_MENU_HOME, u8g2_GetU8x8( &u8g2 )->pins[U8X8_PIN_MENU_HOME], gpio_get_level( u8g2_GetU8x8( &u8g2 )->pins[U8X8_PIN_MENU_HOME] ) );
}

void init_display( ) {
	esp_log_level_set( "u8g2_hal", LOG_u8g2_hal_LEVEL );

	u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
	u8g2_esp32_hal.clk              = (gpio_num_t)PIN_CLK;
	u8g2_esp32_hal.mosi             = (gpio_num_t)PIN_MOSI;
	u8g2_esp32_hal.cs               = (gpio_num_t)PIN_CS;
	u8g2_esp32_hal.dc               = (gpio_num_t)PIN_DC;
	u8g2_esp32_hal.reset            = (gpio_num_t)PIN_RESET;
	u8g2_esp32_hal.menu_select      = (gpio_num_t)MENU_pin_select;
	u8g2_esp32_hal.menu_next        = (gpio_num_t)MENU_pin_next;
	u8g2_esp32_hal.menu_prev        = (gpio_num_t)MENU_pin_prev;
	u8g2_esp32_hal.menu_up          = (gpio_num_t)MENU_pin_up;
	u8g2_esp32_hal.menu_down        = (gpio_num_t)MENU_pin_down;
	u8g2_esp32_hal.menu_home        = (gpio_num_t)MENU_pin_home;
	ESP_LOGD( "init_display", "begin!" );

	u8g2_esp32_hal_init( u8g2_esp32_hal );
	ESP_LOGD( "init_display", "hal init: All done!" );

	// u8x8_SetPin( (&u8g2)->u8x8 , U8X8_PIN_MENU_SELECT, MENU_pin_select );
	// u8g2_GetU8x8(&u8g2)->pins[U8X8_PIN_MENU_SELECT] = MENU_pin_select;
	// u8g2_GetU8x8(&u8g2)->pins[U8X8_PIN_MENU_NEXT]   = MENU_pin_next;
	// u8g2_GetU8x8(&u8g2)->pins[U8X8_PIN_MENU_PREV]   = MENU_pin_prev;
	// u8g2_GetU8x8(&u8g2)->pins[U8X8_PIN_MENU_UP]     = MENU_pin_up;
	// u8g2_GetU8x8(&u8g2)->pins[U8X8_PIN_MENU_DOWN]   = MENU_pin_down;
	// u8g2_GetU8x8(&u8g2)->pins[U8X8_PIN_MENU_HOME]   = MENU_pin_home;

	// u8g2_GetU8x8(&u8g2)->pins[U8X8_PIN_MENU_SELECT] = MENU_pin_select;
	// u8g2_GetU8x8(&u8g2)->pins[U8X8_PIN_MENU_NEXT]   = MENU_pin_next;
	// u8g2_GetU8x8(&u8g2)->pins[U8X8_PIN_MENU_PREV]   = MENU_pin_prev;
	// u8g2_GetU8x8(&u8g2)->pins[U8X8_PIN_MENU_UP]     = MENU_pin_up;
	// u8g2_GetU8x8(&u8g2)->pins[U8X8_PIN_MENU_DOWN]   = MENU_pin_down;
	// u8g2_GetU8x8(&u8g2)->pins[U8X8_PIN_MENU_HOME]   = MENU_pin_home;

	u8g2_Setup_sh1106_128x64_noname_1( &u8g2, U8G2_R0, u8g2_esp32_spi_byte_cb, u8g2_esp32_gpio_and_delay_cb );
	ESP_LOGD( "init_display", "sh1106 init: All done!" );

	u8g2_InitDisplay( &u8g2 );    // send init sequence to the display, display is in sleep mode after this,
	vTaskDelay( 1 );              // <== Insert delay here
	ESP_LOGD( "init_display", "display init: All done!" );

	u8g2_SetPowerSave( &u8g2, 0 );    // wake up display
	ESP_LOGD( "init_display", "powersave: off" );

	display_pins( );
	// uint16_t x1;
	// x1 = 10;
	// u8g2_FirstPage( &u8g2 );
	// do {
	// 	u8g2_DrawBox( &u8g2, 10, 20, 20, 30 );
	// 	u8g2_SetFont( &u8g2, u8g2_font_ncenB14_tr );
	// 	u8g2_DrawStr( &u8g2, 0, 15, "Hello World!" );
	// } while( u8g2_NextPage( &u8g2 ) );
	// ESP_LOGD( "init_display", "All done!" );
}





void UpdateScreen( int centigrade ) {
	// wait for event group update either from button press or from temperature update here
	char buf_temp[16];
	char buf_time[16];
	char buf_ip_addr[20];
	// u8g2.setFont( u8g2_font_logisoso30_tn );
	sprintf( buf_time, "%i", (int)floor( (double)( millis( ) / 1000 ) ) );
	sprintf( buf_temp, "%i \xfe", (int)centigrade );
	wifi_get_local_ip_str( buf_ip_addr );

	u8g2_FirstPage( &u8g2 );
	do {
		u8g2_SetFont( &u8g2, u8g2_font_9x15B_mn );

		// const char
		u8g2_DrawStr( &u8g2, 5, 15, buf_time );
		u8g2_SetFont( &u8g2, u8g2_font_5x7_mn );

		u8g2_DrawStr( &u8g2, 5, 28, buf_ip_addr );
		// u8g2_DrawStr( &u8g2, 40, 31, print_action );
		u8g2_SetFont( &u8g2, u8g2_font_inb30_mr );
		u8g2_DrawStr( &u8g2, 5, 62, buf_temp );
	} while( u8g2_NextPage( &u8g2 ) );


	display_pins( );
	vTaskDelay( pdMS_TO_TICKS( 1000 ) );
	ESP_LOGV( LOGT, "completed screen write" );
}





void ProcessMenuInput( void *arg ) {
	/******************************************
	 ****       check for menu input       ****
	 ******************************************/
	do {
		char   print_action[64];
		int8_t event = u8x8_GetMenuEvent( &u8g2.u8x8 );
		switch( event ) {
			case U8X8_MSG_GPIO_MENU_SELECT:
				sprintf( print_action, "%s", "menu_select" );
				ESP_LOGD( LOGT, "BUTTON PRESS: %s", print_action );
				break;
			case U8X8_MSG_GPIO_MENU_NEXT:
				sprintf( print_action, "%s", "MENU_NEXT" );
				ESP_LOGD( LOGT, "BUTTON PRESS: %s", print_action );
				break;
			case U8X8_MSG_GPIO_MENU_PREV:
				sprintf( print_action, "%s", "MENU_PREV" );
				ESP_LOGD( LOGT, "BUTTON PRESS: %s", print_action );
				break;
			case U8X8_MSG_GPIO_MENU_UP:
				sprintf( print_action, "%s", "MENU_UP" );
				ESP_LOGD( LOGT, "BUTTON PRESS: %s", print_action );
				break;
			case U8X8_MSG_GPIO_MENU_DOWN:
				sprintf( print_action, "%s", "MENU_down" );
				ESP_LOGD( LOGT, "BUTTON PRESS: %s", print_action );
				break;
			case U8X8_MSG_GPIO_MENU_HOME:
				sprintf( print_action, "%s", "MENU_home" );
				ESP_LOGD( LOGT, "BUTTON PRESS: %s", print_action );
				break;
			case 0:
				sprintf( print_action, "%s", "==== 0 ====" );
				ESP_LOGV( LOGT, "BUTTON PRESS: %s", print_action );
				break;
			default:
				sprintf( print_action, "%s %i", "DEFAULT pressed:", event );
				ESP_LOGV( LOGT, "BUTTON PRESS: %s", print_action );
				break;
		}
		vTaskDelay( pdMS_TO_TICKS( 10 ) );

	} while( true );

	// print the string when a newline arrives:
}


#undef LOGT