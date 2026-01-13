#ifndef MY_SOCKET_H
#define MY_SOCKET_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h" 
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include <sys/socket.h>	//提供套接字
#include <netdb.h>//域名和服务器转换 ->ip
#include <errno.h>//C语言标准错误处理
#include <arpa/inet.h>//地址格式和字节序转换
#include "my_wifi_app.h"
#include "esp_timer.h"
#include "ringbuffer.h"
#include "bno08x_app.h"
#include "uart_app.h"
#include "motor_app.h"


typedef enum {
    SOCKET_DISCONNECTED = 0,
    SOCKET_READY = 1
} socket_state_t;

void socket_init_thread(void *pvParameters);   // UDP 初始化线程
void socket_send_thread(void *pvParameters); // UDP发送线程
void socket_rec_thread(void *pvParameters); // UDP接收线程
int my_socket_init(void); // Socket初始化函数
void my_socket_start(void); // 启动所有socket线程

#endif //MY_SOCKET_H