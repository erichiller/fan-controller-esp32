#include <Arduino.h>
// #include <Wire.h>
#include <U8g2lib.h>


#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"

// Comment out the display your NOT using e.g. if you have a 1.3" display comment out the SSD1306 library and object
// #include "SH1106Spi.h"    // https://github.com/squix78/esp8266-oled-ssd1306
// SH1106 display(0x3c, 17,16); // 1.3" OLED display object definition (address, SDA, SCL) Connect OLED SDA , SCL pins to ESP SDA, SCL pins
// SH1106Spi display( 22, 1, 5 );
// #include "font.h"    // The font.h file must be in the same folder as this sketch

U8G2_SH1106_128X64_NONAME_1_4W_HW_SPI u8g2( U8G2_R0, /* cs=*/5, /* dc=*/21, /* reset=*/22 );
// U8G2_SH1106_128X64_NONAME_2_4W_HW_SPI u8g2( U8G2_R0, /* cs=*/5, /* dc=*/21, /* reset=*/22 );
// U8G2_SH1106_128X64_VCOMH0_1_4W_HW_SPI
// U8G2_SH1106_128X64_VCOMH0_2_4W_HW_SPI
// U8G2_SH1106_128X64_WINSTAR_1_4W_HW_SPI u8g2( U8G2_R0, /* cs=*/5, /* dc=*/21, /* reset=*/22 );
// U8G2_SH1106_128X64_WINSTAR_2_4W_HW_SPI
							// 1,2,F

int   sensorPin   = 35;    // select the input pin for the potentiometer
int   sensorValue = 0;     // variable to store the value coming from the sensor
float centigrade  = 0;
float farenheit   = 0;

#define BAUD_RATE 115200


/*** GLOBALS ***/
String inputString = "";    // a String to hold incoming data

/** Menu
 */
uint8_t MENU_pin_select = 4;    // button. must connect other side to ground
uint8_t MENU_pin_next   = 0;    // button. must connect other side to ground
uint8_t MENU_pin_prev   = 2;    // button. must connect other side to ground
uint8_t MENU_pin_up     = 26;


// NOTE:  The most recent version of the encoder from github is required
// for the ESP32: https://github.com/PaulStoffregen/Encoder
// #include "Encoder.h"

// MCPWM Pins
static const unsigned Q = 16;    // GPIO16; THE  ** LED **
// static const unsigned Q_BAR = 8; // GPIO17;

// static mcpwm_dev_t *MCPWM[2] = {&MCPWM0, &MCPWM1};

// Forward declarations
static void init_PWM_based_on_state( void );
static void set_frequency( uint32_t freq );
static void set_duty_cycle( float dc );
static void setup_mcpwm( );
static void setup_mcpwm( );



void setup( ) {
	pinMode( MENU_pin_select, INPUT_PULLDOWN );
	pinMode( MENU_pin_next, INPUT_PULLUP );
	// pinMode( MENU_pin_prev, INPUT_PULLDOWN );
	// pinMode( MENU_pin_up, INPUT_PULLUP );


	// Set up serial port.
	Serial.begin( BAUD_RATE );
	// reserve 200 bytes for the inputString:
	inputString.reserve( 200 );



	setup_mcpwm( );

	init_PWM_based_on_state( );

	u8g2.begin( analogInputToDigitalPin(MENU_pin_select), analogInputToDigitalPin(MENU_pin_next), analogInputToDigitalPin(MENU_pin_prev), MENU_pin_up, U8X8_PIN_NONE, U8X8_PIN_NONE );    // https://github.com/olikraus/u8g2/wiki/u8g2reference#begin
}


enum States {
	DUTY_CYCLE,    ///< Encoder sets duty cycle
	FREQ,          ///< Encoder sets frequency
	MAX_STATES
};                                           // enum States
static int values[MAX_STATES] = {50, 50};    // Initialized duty cycle and frequency to 50%


char buf_temp[16];
char buf_time[16];
char print_action[20] = "none";
void loop( ) {
	/******************************************
	 ****  read the value from the sensor: ****
	 ******************************************/
	sensorValue = analogRead( sensorPin );

	Serial.print( millis( ) );
	Serial.print( " ms || " );
	Serial.print( sensorValue );
	Serial.print( " milliVolt || " );
	Serial.print( centigrade );
	Serial.print( " degrees C || " );

	centigrade = ( sensorValue - 500 ) / 10;
	farenheit  = ( centigrade * 9.0 / 5.0 ) + 32.0;
	Serial.print( farenheit );
	Serial.println( " degrees F" );


	// u8g2.setFont( u8g2_font_logisoso30_tn );
	sprintf( buf_time, "%i", (int)floor( (double)( millis( ) / 1000 ) ) );
	sprintf( buf_temp, "%i \xfe", (int)centigrade );

	u8g2.firstPage( );
	do {
		u8g2.setFont( u8g2_font_9x15B_mn );

		u8g2.setCursor( 5, 15 );
		// const char
		u8g2.print( F( buf_time ) );
		u8g2.setCursor( 5, 31 );
		u8g2.print( F( print_action ) );
		u8g2.setCursor( 5, 62 );
		u8g2.setFont( u8g2_font_inb30_mr );
		u8g2.print( F( buf_temp ) );
	} while( u8g2.nextPage( ) );


	// display.clear( );
	// display.drawString( 0, 0, "125 250 500 1K  2K 4K 8K 16K" );
	// display.display( );
	// delay in ms
	delay( 1000 );

	/******************************************
	 ****       check for menu input       ****
	 ******************************************/
	int8_t event = u8g2.getMenuEvent( );
	switch( event ) {
		case U8X8_MSG_GPIO_MENU_SELECT:
			sprintf( print_action, "%s", "menu_select" );
			Serial.println( print_action );
			break;
		case U8X8_MSG_GPIO_MENU_NEXT:
			sprintf( print_action, "%s", "MENU_NEXT" );
			Serial.println( print_action );
			break;
		case U8X8_MSG_GPIO_MENU_PREV:
			sprintf( print_action, "%s", "MENU_PREV" );
			Serial.println( print_action );
			break;
		case U8X8_MSG_GPIO_MENU_UP:
			sprintf( print_action, "%s", "MENU_UP" );
			Serial.println( print_action );
			break;
		case U8X8_MSG_GPIO_MENU_DOWN:
			sprintf( print_action, "%s", "MENU_down" );
			Serial.println( print_action );
			break;
		case U8X8_MSG_GPIO_MENU_HOME:
			sprintf( print_action, "%s", "MENU_home" );
			Serial.println( print_action );
			break;
		default:
			sprintf( print_action, "%s %i", "DEFAULT pressed:", event );
			Serial.println( print_action );
			break;
	}

	Serial.print("MENU_pin_select == ");
	if( digitalRead( MENU_pin_select ) == LOW ) {
		Serial.print( "LOW" );
	}
	if( digitalRead( MENU_pin_select ) == HIGH ) {
		Serial.print( "HIGH" );
	}
	Serial.println("");
	Serial.print("MENU_pin_next == ");	
	if( digitalRead( MENU_pin_next ) == LOW ) {
		Serial.print( "LOW" );
	}
	if( digitalRead( MENU_pin_next ) == HIGH ) {
		Serial.print( "HIGH" );
	}
	Serial.println("");
	Serial.print("MENU_pin_prev == ");	
	if( digitalRead( MENU_pin_prev ) == LOW ) {
		Serial.print( "LOW" );
	}
	if( digitalRead( MENU_pin_prev ) == HIGH ) {
		Serial.print( "HIGH" );
	}
	Serial.println("");
	Serial.print("MENU_pin_up == ");	
	if( digitalRead( MENU_pin_up ) == LOW ) {
		Serial.print( "LOW" );
	}
	if( digitalRead( MENU_pin_up ) == HIGH ) {
		Serial.print( "HIGH" );
	}
	Serial.println("");


	// print the string when a newline arrives:
	while( Serial.available( ) ) {
		Serial.println( "Serial data available" );
		char inChar = (char)Serial.read( );
		inputString += inChar;
		if( inChar == ';' || inChar == '\n' ) {
			Serial.println( inputString );
			if( inputString.substring( 0, 3 ) == "GET" ) {
			}
			if( inputString.substring( 0, 3 ) == "SET" ) {
				Serial.println( "SET" );

				if( inputString.substring( 4, 8 ) == "duty" ) {
					Serial.println( "duty" );

					values[0] = inputString.substring( 9 ).toInt( );
					Serial.println( values[0] );

					init_PWM_based_on_state( );
				}
			}





			// clear the string:
			inputString = "";
		}
	}
}


//    static unsigned state = 0U;
//    static int last_val_in_state = 0;
//    static bool first_time_through_loop = true;

//    if ( first_time_through_loop )
//    {
// 	  encoder.write(values[state]);
// 	  first_time_through_loop = false;
// 	  last_val_in_state = values[state];
//    }

//    values[state] = encoder.read();

//    if ( button_cnt > 0 )
//    {
// 	  // button has been pressed
// 	  state++;
// 	  if (state > MAX_STATES-1)
// 		 state = 0;

// 	  encoder.write(values[state]);
// 	  last_val_in_state = values[state];

// 	  button_cnt = 0;
//    }

//    // React to whatever changes have been made.
//    if ( values[state] != last_val_in_state )
//    {
// 	  switch(state)
// 	  {
// 	  case DUTY_CYCLE : values[state] = enc_set_dc(values[state]);
// 		 break;
// 	  case FREQ : values[state] = enc_set_freq(values[state]);
// 		 break;
// 	  } // switch(state)

// 	  last_val_in_state = values[state];
// 	  encoder.write(values[state]);
//    }

//    Serial.println("State: " + String(state)
// 				  + " Value: " + String(values[state]));

//    delay(500);

// } // loop()

static int enc_set_dc( int val ) {
	// val is a percentage
	// Returns val
	static constexpr int MAX = {100};
	static constexpr int MIN = {10};

	// Clamp the value
	int pct = ( val >= MIN ) ? val : MIN;
	pct     = ( pct <= MAX ) ? pct : MAX;

	// Compute the controlling values.
	set_duty_cycle( pct / 2.0 );
	Serial.println( "Duty Cycle: " + String( pct / 2.0 ) );

	return pct;
}    // enc_set_dc()

static int enc_set_freq( int val ) {
	// val is a percentage.
	// Note that setting dy will change frequency because it changes the slope
	// and the number of steps per cycle is determined by the slope and the
	// amplitude.
	static constexpr int      MAX      = {100};
	static constexpr int      MIN      = {0};
	static constexpr uint32_t MIN_FREQ = {500};
	static constexpr uint32_t MAX_FREQ = {10000};

	// Clamp the value
	int pct = ( val >= MIN ) ? val : MIN;
	pct     = ( pct <= MAX ) ? pct : MAX;

	// Compute the new frequency
	int new_freq = static_cast<int>( ( pct / 100.0 ) * ( MAX_FREQ - MIN_FREQ ) ) + MIN_FREQ;

	Serial.println( "Freq: " + String( new_freq ) );

	set_frequency( new_freq );

	return pct;
}    // enc_set_freq()

static void init_PWM_based_on_state( ) {
	values[DUTY_CYCLE] = enc_set_dc( values[DUTY_CYCLE] );
	values[FREQ]       = enc_set_freq( values[FREQ] );
}    // init_PWM


static void setup_mcpwm_pins( ) {
	Serial.println( "initializing mcpwm control gpio...n" );
	mcpwm_gpio_init( MCPWM_UNIT_0, MCPWM0A, Q );
	//    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, Q_BAR);
}    // setup_pins()


static void setup_mcpwm( ) {
	setup_mcpwm_pins( );

	Serial.println( "Configuring initial parameters ...\n" );
	// Not that this is initial configuration only, further, more
	// detailed configuration will often be required.
	mcpwm_config_t pwm_config;
	pwm_config.frequency = 10000;    //frequency = 1000Hz
	pwm_config.cmpr_a    = 30.0;     //duty cycle of PWMxA = 30.0%
	// pwm_config.cmpr_b       = 30.0;                            //duty cycle of PWMxB = 30.0%
	pwm_config.counter_mode = MCPWM_UP_DOWN_COUNTER;           // Up-down counter (triangle wave)
	pwm_config.duty_mode    = MCPWM_DUTY_MODE_0;               // Active HIGH
	mcpwm_init( MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config );    //Configure PWM0A & PWM0B with above settings

	// The following calls set what happens on transitions relative to the
	// comparators.  The API documentation doesn't make this clear, but
	// I believe that these set the 'actions' described under "PWM Signal Generation"
	// in section 15.3.3 on pg. 341 of the reference manual (v2.7).  What is odd is
	// that 'toggle' is not supported.  I don't need these for my application, but
	// I did experiment with them and they work.
	// mcpwm_set_signal_low(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A);
	// mcpwm_set_signal_high(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B);

	// MCPWMXA to duty mode 1 and MCPWMXB to duty mode 0 or vice versa
	// will generate MCPWM compliment signal of each other, there are also
	// other ways to do it
	mcpwm_set_duty_type( MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0 );
	// mcpwm_set_duty_type( MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, MCPWM_DUTY_MODE_1 );

	// Allows the setting of a minimum dead time between transitions to prevent
	// "shoot through" a totem-pole switch pair, which can be disastrous.
	//	mcpwm_deadtime_enable(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_BYPASS_FED, 100, 100);   //Deadtime of 10us

	mcpwm_start( MCPWM_UNIT_0, MCPWM_TIMER_0 );
}    // setup_mcpwm

static void set_duty_cycle( float dc ) {
	// dc is the duty cycle, as a percentage (not a fraction.)
	// (e.g. 30.0 is 30%)
	mcpwm_set_duty( MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, dc );
	// mcpwm_set_duty( MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, 100.0 - dc );
}    // set_duty_cycle()

static void set_frequency( uint32_t freq ) {
	// freq is the desired frequency, in Hz.
	mcpwm_set_frequency( MCPWM_UNIT_0, MCPWM_TIMER_0, freq * 2U );
}    // Set the PWM frequency.

// void buttonISR( void ) {
// button_cnt++;
// }    // buttonISR()
