#ifndef PWM_H
#define PWM_H

#include <math.h>
#include <driver/ledc.h>

extern uint32_t  PWM_PERCENT;

extern ledc_channel_config_t ledc_channel;

void set_duty_percent( int percent );
void set_ledc_channel_config( );

#endif