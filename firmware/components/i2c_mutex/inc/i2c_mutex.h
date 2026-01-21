#ifndef I2C_MUTEX_H
#define I2C_MUTEX_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

extern SemaphoreHandle_t i2c_bus_mutex;

void i2c_mutex_init(void);

#define I2C_LOCK()   xSemaphoreTake(i2c_bus_mutex, portMAX_DELAY)
#define I2C_UNLOCK() xSemaphoreGive(i2c_bus_mutex)

#endif
