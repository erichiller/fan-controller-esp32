#ifndef TEMP_H
#define TEMP_H

extern float          centigrade;
extern float          farenheit;

void SensorTask( void *arg );

uint32_t measure_adc_multisample_raw( );
void init_sensor();


#endif