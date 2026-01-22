#ifndef MOTOR_APP_H
#define MOTOR_APP_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "motor_encoder.h"  
#include "tb6612_motor.h"
#include "my_pid.h"
#include "key_app.h"

extern motor_encoder_t left_encoder;
extern motor_encoder_t right_encoder;
extern tb6612_motor_t left_motor;
extern tb6612_motor_t right_motor;
extern PID_LocTypeDef left_motor_pid;
extern PID_LocTypeDef right_motor_pid;

void motor_app_init(void);
void motor_speed_ring_task(void *pvParameters);

#endif // !MOTOR_APP_H
