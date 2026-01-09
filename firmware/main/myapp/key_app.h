#ifndef KEY_APP_H
#define KEY_APP_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "key.h"

extern uint8_t key_count;
extern SemaphoreHandle_t key_semaphore; //按键信号量

void key_app_init(void);
void key_app_task(void *pvParameters);

#endif // !KEY_APP_H
