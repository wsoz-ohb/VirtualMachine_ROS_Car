#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h" 
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_wifi.h"


#include "my_wifi_app.h"
#include "my_socket.h"
#include "motor_app.h"
#include "bno08x_app.h"
#include "my_oled.h"
#include "key_app.h"
#include "uart_app.h"

#define TAG "main"

void app_main(void)
{
    key_app_init(); //初始化按键
    motor_app_init(); //初始化电机
    bno08x_app_init(); //初始化BNO08X传感器
    uart_app_init(); //初始化UART串口
    my_wifi_app_start(); //启动WiFi
    my_socket_start(); //启动UDP socket线程


    // OLED_Init(); //初始化OLED显示屏
    // OLED_Clear(); //清屏
    // OLED_ShowStr(0,0,"Hello World!",OLED_FONT_SIZE_6X8); //显示字符串
    while (1)
    {

        ESP_LOGI(TAG, "imu data: %f, %f, %f\r\n",bno08x_data.gyro_x, bno08x_data.gyro_y, bno08x_data.gyro_z);
        ESP_LOGI(TAG, "imu data2: %f, %f, %f, %f\r\n",bno08x_data.quat_i, bno08x_data.quat_j, bno08x_data.quat_k, bno08x_data.quat_real);
        ESP_LOGI(TAG, "imu data3: %f, %f, %f\r\n",bno08x_data.acc_x, bno08x_data.acc_y, bno08x_data.acc_z);
        ESP_LOGI(TAG, "imu data4: %f, %f, %f\r\n",bno08x_data.pitch, bno08x_data.roll, bno08x_data.yaw);
        ESP_LOGI(TAG, "left encoder: %f, right encoder: %f\r\n",left_encoder.rpm, right_encoder.rpm);
        ESP_LOGI(TAG, "Car is running...");
        vTaskDelay(500 / portTICK_PERIOD_MS);

    }

}
