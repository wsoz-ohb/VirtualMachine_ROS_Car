#include "i2c_mutex.h"

SemaphoreHandle_t i2c_bus_mutex = NULL;

void i2c_mutex_init(void)
{
    if (i2c_bus_mutex == NULL) {
        i2c_bus_mutex = xSemaphoreCreateMutex();
    }
}
