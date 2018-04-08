# Pump controller for deepthought

1. Read temperature from TMP36
2. Display on SH1106 display
3. PWM output to PUMP
4. (Button input to control PUMP)

# ESP32 Core 2

Uses `CP210x_Universal_Windows_Driver` for USB->Serial chip

[Pin Diagram](https://github.com/espressif/arduino-esp32/blob/master/docs/esp32_pinmap.png)


# TMP36 temperature sensor
[TMP36 Documentation](https://cdn.sparkfun.com/datasheets/Sensors/Temp/TMP35_36_37.pdf)

[TMP36 on Adafruit](https://www.adafruit.com/product/165)  
    **Pins:**
Viewing from the flat face towards me, left to right:
1. (left) power (between 2.7 and 5.5V)
2. (center) analog in on your microcontroller.
3. (right) ground

The voltage out is 0V at -50°C and 1.75V at 125°C. You can easily calculate the temperature from the voltage in millivolts:  
    Temp °C = 100*(reading in V) - 50

[Tutorial on Adafruit](https://learn.adafruit.com/tmp36-temperature-sensor)

# SPI

ESP32   | SH1106    | Notes
---     | ---       | ---
MOSI    | MOSI      | Master in Slave out
SS      | RES       | Reset line
        | DC        | Data Command


[SPI tutorial on Sparkfun](https://learn.sparkfun.com/tutorials/serial-peripheral-interface-spi#receiving-data)