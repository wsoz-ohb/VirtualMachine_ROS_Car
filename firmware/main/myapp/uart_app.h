#ifndef UART_APP_H
#define UART_APP_H

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"

/* 串口配置参数 */
#define UART_PORT_NUM           UART_NUM_1          // 使用UART1
#define UART_BAUD_RATE          115200              // 波特率
#define UART_TX_PIN             17                  // TX引脚
#define UART_RX_PIN             18                  // RX引脚

#define UART_BUF_SIZE           512                 // UART驱动缓冲区大小
#define UART_RX_DMABUF_SIZE     512                 // UART接收临时缓冲区
#define UART_RINGBUF_SIZE       4096                // 环形缓冲区大小（约355ms数据）


extern QueueHandle_t lidar_queue; //雷达数据队列句柄
typedef struct {
    uint16_t length;          // 数据长度
    uint8_t data[UART_BUF_SIZE]; // 数据内容
} lidar_packet_t; // 雷达数据包结构体
/**
 * @brief 初始化UART应用
 * @note 会初始化UART硬件、环形缓冲区，并创建UART事件处理任务和数据处理任务
 */
void uart_app_init(void);

/**
 * @brief 格式化串口发送函数
 * @param format 格式化字符串（类似printf）
 * @param ... 可变参数
 * @return
 *      - 发送的字节数: 成功
 *      - -1: 失败
 */
int uart_printf(const char* format, ...);

/**
 * @brief 获取环形缓冲区数据长度
 * @return 环形缓冲区中的数据字节数
 */
uint16_t uart_get_data_len(void);

#endif // !UART_APP_H
