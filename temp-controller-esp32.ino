#include <Arduino.h>
#include <Wire.h>


// Comment out the display your NOT using e.g. if you have a 1.3" display comment out the SSD1306 library and object
#include "SH1106.h"     // https://github.com/squix78/esp8266-oled-ssd1306
SH1106 display(0x3c, 17,16); // 1.3" OLED display object definition (address, SDA, SCL) Connect OLED SDA , SCL pins to ESP SDA, SCL pins
#include "font.h" // The font.h file must be in the same folder as this sketch



/**
 *
class U8G2_SH1106_128X64_NONAME_1_4W_HW_SPI : public U8G2 {
  public: U8G2_SH1106_128X64_NONAME_1_4W_HW_SPI(const u8g2_cb_t *rotation, uint8_t cs, uint8_t dc, uint8_t reset = U8X8_PIN_NONE) : U8G2() {
    u8g2_Setup_sh1106_128x64_noname_1(&u8g2, rotation, u8x8_byte_arduino_hw_spi, u8x8_gpio_and_delay_arduino);
    u8x8_SetPin_4Wire_HW_SPI(getU8x8(), cs, dc, reset);
  }
};
 */
int   sensorPin   = A0;    // select the input pin for the potentiometer
int   sensorValue = 0;     // variable to store the value coming from the sensor
float centigrade  = 0;
float farenheit   = 0;

// attach a LED to pPIO 21
#define LED_PIN 21

#define BAUD_RATE 115200


/*** GLOBALS ***/
String  inputString    = "";       // a String to hold incoming data
boolean stringComplete = false;    // whether the string is complete

void setup( ) {
	// Set up serial port.
	Serial.begin( BAUD_RATE );
	// reserve 200 bytes for the inputString:
	inputString.reserve( 200 );
	u8g2.begin( );
  // ledc_timer_
}

void loop( ) {
	// read the value from the sensor:
	sensorValue = analogRead( sensorPin );

	Serial.print( sensorValue );
	Serial.print( " milliVolt || " );
	Serial.print( centigrade );
	Serial.print( " degrees C || " );

	centigrade = ( sensorValue - 500 ) / 10;
	farenheit  = ( centigrade * 9.0 / 5.0 ) + 32.0;
	Serial.print( farenheit );
	Serial.println( " degrees F" );

	u8g2.clearBuffer( );                      // clear the internal memory
	u8g2.setFont( u8g2_font_ncenB08_tr );     // choose a suitable font
	u8g2.drawStr( 0, 10, "Hello World!" );    // write something to the internal memory
	u8g2.sendBuffer( );                       // transfer internal memory to the display
	// delay in ms
	delay( 1000 );

	// print the string when a newline arrives:
	while( Serial.available( ) ) {
		Serial.println( "Serial data available" );
		char inChar = (char)Serial.read( );
		inputString += inChar;
		if( inChar == ';' || inChar == '\n' ) {
			Serial.println( inputString );
			// clear the string:
			inputString    = "";
			stringComplete = false;
		}
	}
}

void pwm() {

}
