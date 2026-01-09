#ifndef TB6612_H
#define TB6612_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h" 
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/ledc.h"

typedef enum
{
    TB6612_MOTOR_DIR_NORMAL,
    TB6612_MOTOR_DIR_UNNORMAL,
}tb6612_motor_dir_t;

typedef struct 
{
    tb6612_motor_dir_t dir_mode;
    uint8_t tb6612_pwm_gpio; 
    uint8_t tb6612_in1_gpio;
    uint8_t tb6612_in2_gpio;
    ledc_channel_t pwm_channel;
}tb6612_motor_t;

void tb6612_gpio_register(tb6612_motor_t* motor);
void tb6612_motor_pwm_set(tb6612_motor_t* motor, int32_t duty);
void tb6612_motor_direction_set(tb6612_motor_t* motor, tb6612_motor_dir_t direction);

#endif // !TB6612_H
