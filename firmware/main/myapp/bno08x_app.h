#ifndef BNO08X_APP_H
#define BNO08X_APP_H

#include "bno08x.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
extern QueueHandle_t imu_queue; //定义队列句柄



typedef struct {
    // 欧拉角 (Euler Angles)
    float roll;     // 横滚角
    float yaw;      // 偏航角  
    float pitch;    // 俯仰角
    
    // 线性加速度 (LineAcceleration) - 3轴
    float acc_x;    // X轴加速度
    float acc_y;    // Y轴加速度
    float acc_z;    // Z轴加速度
    
    // 角速度 (Angular Velocity) - 3轴
    float gyro_x;   // X轴角速度
    float gyro_y;   // Y轴角速度
    float gyro_z;   // Z轴角速度
    
    // 旋转向量/四元数 (Rotation Vector/Quaternion)
    float quat_i;   
    float quat_j;   
    float quat_k;   
    float quat_real;   
    
    // 可选：磁力计数据 (Magnetometer)
    float mag_x;    // X轴磁场强度
    float mag_y;    // Y轴磁场强度
    float mag_z;    // Z轴磁场强度
    
    uint8_t scl_gpio;   // I2C SCL引脚
    uint8_t sda_gpio;    // I2C SDA引脚
} bno08x_data_t;

extern bno08x_data_t bno08x_data; //定义一个全局变量用于存储传感器数据

void bno08x_app_init(void);
void get_data(void);

#endif // BNO08X_APP_H
