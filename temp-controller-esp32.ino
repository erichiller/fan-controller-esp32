#include <Arduino.h>
#include <WiFi.h>
#include <U8g2lib.h>


#include "driver/ledc.h"
#include "soc/ledc_reg.h"
#include "soc/ledc_struct.h"

#include "driver/adc.h"
#include "esp_adc_cal.h"

#include "secrets.h"

#define DUTY_RESOLUTION LEDC_TIMER_11_BIT    // 11 bits gives me 0-2047 levels of resolution
#define PWM_FREQUENCY 25000
#define commandBuf_MAX 64


U8G2_SH1106_128X64_NONAME_1_4W_HW_SPI u8g2( U8G2_R0, /* cs=*/5, /* dc=*/21, /* reset=*/22 );

int   sensorPin   = 33;    // select the input pin for the Sensor :: Pin 35 is ADC1_CHANNEL_7
int   sensorValue = 0;     // variable to store the value coming from the sensor
float centigrade  = 0;
float farenheit   = 0;

int      PWM_PIN     = 19;
uint32_t DUTY_MAX    = pow( 2, (double)DUTY_RESOLUTION );
uint32_t PWM_PERCENT = 50;

ledc_channel_config_t ledc_channel = {
    .gpio_num   = PWM_PIN,
    .speed_mode = LEDC_HIGH_SPEED_MODE,
    .channel    = LEDC_CHANNEL_0,
    .intr_type  = LEDC_INTR_DISABLE,
    .timer_sel  = LEDC_TIMER_0,
    .duty       = ( uint32_t )( ( (double)PWM_PERCENT / 100 ) * (double)DUTY_MAX )

};

ledc_timer_config_t ledc_timer = {};

#define BAUD_RATE 115200

#define WIFI_TIMEOUT 15000    // 15s timeout
WiFiServer server( 80 );


/*** GLOBALS ***/
char commandBuf[commandBuf_MAX]     = "";    // a String to hold incoming data

/** Menu
 */
uint8_t MENU_pin_select = 25;    // button. must connect other side to ground
uint8_t MENU_pin_next   = 26;    // button. must connect other side to ground
uint8_t MENU_pin_prev   = 32;    // button. must connect other side to ground
uint8_t MENU_pin_up     = 4;
uint8_t MENU_pin_down   = 12;
uint8_t MENU_pin_home   = 14;

// Forward declarations
void set_duty_percent( int percent );



void setup( ) {
	ledc_timer.duty_resolution = LEDC_TIMER_11_BIT;       // resolution of PWM duty
	ledc_timer.freq_hz         = PWM_FREQUENCY;           // frequency of PWM signal
	ledc_timer.speed_mode      = LEDC_HIGH_SPEED_MODE;    // timer mode
	ledc_timer.timer_num       = LEDC_TIMER_0;            // timer index
	ledc_timer_config( &ledc_timer );
	ledc_channel_config( &ledc_channel );

	// Set up serial port.
	Serial.begin( BAUD_RATE );

	int wifi_begin = millis( );
	WiFi.begin( ssid, password );

	while( WiFi.status( ) != WL_CONNECTED && !WiFi.localIP( ) && millis( ) < wifi_begin + 15000 ) {
		delay( 500 );
		Serial.print( "." );
	}

	Serial.println( "\n:LEDC INIT VALUES\nledc_timer.duty_resolution:\t" + String( ledc_timer.duty_resolution ) + "\n"
	                                                                                                              "ledc_channel.freq_hz:\t" +
	                String( ledc_timer.freq_hz ) + "\n" +
	                "ledc_channel.duty:\t" + ledc_channel.duty + "\n"
	                                                             "DUTY_MAX:\t" +
	                DUTY_MAX + "\n" );

	pinMode( MENU_pin_select, INPUT_PULLUP );
	pinMode( MENU_pin_next, INPUT_PULLUP );
	pinMode( MENU_pin_prev, INPUT_PULLUP );
	pinMode( MENU_pin_up, INPUT_PULLUP );
	pinMode( MENU_pin_down, INPUT_PULLUP );
	pinMode( MENU_pin_home, INPUT_PULLUP );

	Serial.println( "" );
	Serial.println( "WiFi connected." );
	Serial.println( "IP address: " );
	Serial.println( WiFi.localIP( ) );
	server.begin( );

	// u8g2.begin( analogInputToDigitalPin( MENU_pin_select ), analogInputToDigitalPin( MENU_pin_next ), analogInputToDigitalPin( MENU_pin_prev ), analogInputToDigitalPin( MENU_pin_up ), MENU_pin_down, MENU_pin_home );    // https://github.com/olikraus/u8g2/wiki/u8g2reference#begin
	u8g2.begin( MENU_pin_select, MENU_pin_next, MENU_pin_prev, MENU_pin_up, MENU_pin_down, MENU_pin_home );    // https://github.com/olikraus/u8g2/wiki/u8g2reference#begin

	Serial.println( "pins set" );
}

char buf_temp[16];
char buf_time[16];
char print_action[20] = "none";
void loop( ) {
	// Serial.println( "creating client wifi" );
	WiFiClient client = server.available( );    // listen for incoming clients

	if( client ) {
		Serial.println( "New Client" );
		String currentLine = "";
		while( client.connected( ) ) {
			if( client.available( ) ) {
				char c = client.read( );
				Serial.write( c );
				if( c == '\n' ) {
					// if the current line is blank, you got two newline characters in a row.
					// that's the end of the client HTTP request, so send a response:
					if( currentLine.length( ) == 0 ) {
						// HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
						// and a content-type so the client knows what's coming, then a blank line:
						client.println( "HTTP/1.1 200 OK" );
						client.println( "Content-type:text/html" );
						client.println( );

						// the content of the HTTP response follows the header:
						client.print( "<b>STATUS:</b> " );
						client.print( ledc_channel.duty );
						client.print( "<br /><br /><a href=\"/H\">UP</a><a href=\"/HT\">+10</a><br />>" );
						client.print( "<a href=\"/L\">DOWN</a><a href=\"/LT\">-10</a><br>" );

						// The HTTP response ends with another blank line:
						client.println( );
						// break out of the while loop:
						break;
					} else {    // if you got a newline, then clear currentLine:
						currentLine = "";
					}
				} else if( c != '\r' ) {    // if you got anything else but a carriage return character,
					currentLine += c;       // add it to the end of the currentLine
				}

				// Check to see if the client request was "GET /H" or "GET /L":
				if( currentLine.endsWith( "GET /H" ) ) {
					// digitalWrite( 5, HIGH );    // GET /H turns the LED on
					set_duty_percent( PWM_PERCENT + 1 );
				}
				if( currentLine.endsWith( "GET /HT" ) ) {
					// digitalWrite( 5, HIGH );    // GET /H turns the LED on
					set_duty_percent( PWM_PERCENT + 10 );
				}
				if( currentLine.endsWith( "GET /L" ) ) {
					set_duty_percent( PWM_PERCENT - 1 );
					// digitalWrite( 5, LOW );    // GET /L turns the LED off
				}
				if( currentLine.endsWith( "GET /LT" ) ) {
					set_duty_percent( PWM_PERCENT - 10 );
					// digitalWrite( 5, LOW );    // GET /L turns the LED off
				}
			}
		}
		// close the connection:
		client.stop( );
		Serial.println( "Client Disconnected." );
	}
	/******************************************
	 ****  read the value from the sensor: ****
	 ******************************************/
	analogRead( sensorPin );
	delay(10);
	sensorValue = analogRead( sensorPin );

	// Serial.print( millis( ) );
	// Serial.print( " ms || " );
	// Serial.print( sensorValue );
	// Serial.print( " milliVolt || " );
	// Serial.print( centigrade );
	// Serial.print( " degrees C || " );

	centigrade = ( sensorValue - 500 ) / 10;
	farenheit  = ( centigrade * 9.0 / 5.0 ) + 32.0;
	// Serial.print( farenheit );
	// Serial.println( " degrees F" );

	measure_adc_multisample_raw();



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


	delay( 1000 );

	/******************************************
	 ****       check for menu input       ****
	 ******************************************/
	/*
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
		case 0:
			sprintf( print_action, "%s", "==== 0 ====" );
			Serial.println( print_action );
			break;
		default:
			sprintf( print_action, "%s %i", "DEFAULT pressed:", event );
			Serial.println( print_action );
			break;
	}

	Serial.print( "MENU_pin_select == " );
	if( digitalRead( MENU_pin_select ) == LOW ) {
		Serial.print( "LOW" );
	}
	if( digitalRead( MENU_pin_select ) == HIGH ) {
		Serial.print( "HIGH" );
	}
	Serial.println( "" );
	Serial.print( "MENU_pin_next == " );
	if( digitalRead( MENU_pin_next ) == LOW ) {
		Serial.print( "LOW" );
	}
	if( digitalRead( MENU_pin_next ) == HIGH ) {
		Serial.print( "HIGH" );
	}
	Serial.println( "" );
	Serial.print( "MENU_pin_prev == " );
	if( digitalRead( MENU_pin_prev ) == LOW ) {
		Serial.print( "LOW" );
	}
	if( digitalRead( MENU_pin_prev ) == HIGH ) {
		Serial.print( "HIGH" );
	}
	Serial.println( "" );
	Serial.print( "MENU_pin_up == " );
	if( digitalRead( MENU_pin_up ) == LOW ) {
		Serial.print( "LOW" );
	}
	if( digitalRead( MENU_pin_up ) == HIGH ) {
		Serial.print( "HIGH" );
	}
	Serial.println( "" );
	Serial.print( "MENU_pin_down == " );
	if( digitalRead( MENU_pin_down ) == LOW ) {
		Serial.print( "LOW" );
	}
	if( digitalRead( MENU_pin_down ) == HIGH ) {
		Serial.print( "HIGH" );
	}
	Serial.println( "" );
	Serial.print( "MENU_pin_home == " );
	if( digitalRead( MENU_pin_home ) == LOW ) {
		Serial.print( "LOW" );
	}
	if( digitalRead( MENU_pin_home ) == HIGH ) {
		Serial.print( "HIGH" );
	}
	Serial.println( "" );
**/

	// print the string when a newline arrives:


	// Process any incoming characters from the serial port
	while( Serial.available( ) > 0 ) {
		char c = Serial.read( );
		// Add any characters that aren't the end of a command (semicolon) to the input buffer.
		if( c != ';' && c != '\n' ) {
			c = toupper( c );
			strncat( commandBuf, &c, 1 );
		} else {
			// Parse the command because an end of command token was encountered.
			parseCommand( commandBuf );

			// clear the string:
			memset(commandBuf, 0 , commandBuf_MAX);
		}
	}
}

void parseCommand( char *commandBuf ) {
	Serial.println("parsing command: " + (String)commandBuf);
	if( strcmp(commandBuf,"SET DUTY" ) == 0 ) {
		int new_pwm_percent = ((String)commandBuf).substring( 9 ).toInt( );
		Serial.println( "Setting new duty cycle percentage to:" + new_pwm_percent );
		set_duty_percent( new_pwm_percent );
	}
	// clear the string:
	commandBuf = "";
}

int NO_OF_SAMPLES = 64;
adc_unit_t ADC_UNIT = ADC_UNIT_1;


void measure_adc_multisample_raw(){
	static esp_adc_cal_characteristics_t *adc_chars;

	adc_chars = (esp_adc_cal_characteristics_t*)calloc(1, sizeof(esp_adc_cal_characteristics_t));
	esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT, ADC_ATTEN_11db, ADC_WIDTH_BIT_12, 1100, adc_chars);
	//Check type of calibration value used to characterize ADC
	Serial.println(val_type);
	//Continuously sample ADC1
	while (1) {
		uint32_t adc_reading = 0;
		//Multisampling
		for (int i = 0; i < NO_OF_SAMPLES; i++) {
			if (ADC_UNIT == ADC_UNIT_1) {
				adc_reading += adc1_get_raw((adc1_channel_t)sensorPin);
			} else {
				int raw;
				adc2_get_raw((adc2_channel_t)sensorPin, ADC_WIDTH_BIT_12, &raw);
				adc_reading += raw;
			}
		}
		adc_reading /= NO_OF_SAMPLES;
		//Convert adc_reading to voltage in mV
		uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
		printf("Raw: %d\tVoltage: %dmV\n", adc_reading, voltage);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}

}



void set_duty_percent( int percent ) {
	// returns duty in fractional
	static constexpr int MAX = {100};
	static constexpr int MIN = {20};

	// Clamp the value
	percent = ( percent >= MIN ) ? percent : MIN;
	percent = ( percent <= MAX ) ? percent : MAX;

	// Compute the controlling values.
	uint32_t duty = ( uint32_t )( ( (double)percent / 100 ) * (double)DUTY_MAX );

	Serial.println( "Setting Duty Cycle: " + String( percent ) + "% ; " + String( duty ) + "/" + DUTY_MAX );

	ledc_set_duty( ledc_channel.speed_mode, ledc_channel.channel, duty );
	ledc_update_duty( ledc_channel.speed_mode, ledc_channel.channel );
	PWM_PERCENT = percent;
}    // set_duty_percent()
