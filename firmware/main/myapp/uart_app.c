/**
 * @file uart_app.c
 * @brief ESP32-S3串口应用模块，使用环形缓冲区实现数据接收和处理
 * @note 参考STM32 HAL的DMA+空闲中断模式
 */

#include "uart_app.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "ringbuffer.h"
#include <string.h>

static const char *TAG = "uart_app";

/* 静态变量定义 - 类似STM32的全局变量 */
static uint8_t uart_rx_dmabuffer[UART_RX_DMABUF_SIZE];     // 空闲中断数据缓存（类似STM32 DMA缓冲）
static uint8_t uart_read_buffer[UART_RX_DMABUF_SIZE];      // 用户读取缓冲区

static struct rt_ringbuffer uart_ringbuffer_struct;         // 串口环形缓冲区结构体
static rt_uint8_t uart_ringbuffer[UART_RINGBUF_SIZE];      // 环形缓冲区实际大小

static QueueHandle_t uart_event_queue = NULL;              // UART事件队列

/**
 * @brief UART事件处理任务
 * @note 类似于STM32的HAL_UARTEx_RxEventCallback()回调函数
 *       负责将UART接收到的数据放入环形缓冲区
 */
static void uart_event_task(void *pvParameters)
{
    uart_event_t event;

    while (1) {
        // 等待UART事件（类似等待DMA传输完成中断）
        if (xQueueReceive(uart_event_queue, (void *)&event, portMAX_DELAY)) {
            switch (event.type) {
                case UART_DATA:
                    // 接收到数据事件（类似DMA+空闲中断触发）
                    ESP_LOGD(TAG, "UART_DATA event, size: %d", event.size);

                    // 读取UART驱动缓冲区中的数据到临时缓冲区
                    // 类似STM32的DMA已经搬运完成，我们从DMA缓冲区读取
                    int len = uart_read_bytes(UART_PORT_NUM, uart_rx_dmabuffer,
                                             event.size, pdMS_TO_TICKS(100));

                    if (len > 0) {
                        // 判断缓冲区空间（类似STM32代码）
                        if (rt_ringbuffer_space_len(&uart_ringbuffer_struct) != 0) {
                            // 将数据放入环形缓冲区
                            uint16_t putsize = rt_ringbuffer_put(&uart_ringbuffer_struct,
                                                                uart_rx_dmabuffer, len);

                            if (putsize != len) {
                                // 环形缓冲区数据未全部放入
                                ESP_LOGW(TAG, "Ringbuffer Size too Small, lost %d bytes",
                                        len - putsize);
                            }
                        } else {
                            ESP_LOGW(TAG, "Ringbuffer is full, data lost");
                        }
                    }
                    break;

                case UART_FIFO_OVF:
                    ESP_LOGW(TAG, "UART FIFO overflow");
                    uart_flush_input(UART_PORT_NUM);
                    xQueueReset(uart_event_queue);
                    break;

                case UART_BUFFER_FULL:
                    ESP_LOGW(TAG, "UART ring buffer full");
                    uart_flush_input(UART_PORT_NUM);
                    xQueueReset(uart_event_queue);
                    break;

                case UART_BREAK:
                    ESP_LOGD(TAG, "UART break detected");
                    break;

                case UART_PARITY_ERR:
                    ESP_LOGE(TAG, "UART parity error");
                    break;

                case UART_FRAME_ERR:
                    ESP_LOGE(TAG, "UART frame error");
                    break;

                default:
                    ESP_LOGW(TAG, "UART event type: %d", event.type);
                    break;
            }
        }
    }
}

/**
 * @brief UART数据处理任务
 * @note 类似STM32的uart1_task()，从环形缓冲区读取数据并处理
 */
static void uart_app_task(void *pvParameters)
{
    while (1) {
        // 获取缓冲区数据大小（类似STM32代码）
        uint16_t data_size = rt_ringbuffer_data_len(&uart_ringbuffer_struct);

        if (data_size > 0) {
            // 从环形缓冲区读取数据
            memset(uart_read_buffer, 0, sizeof(uart_read_buffer));
            rt_ringbuffer_get(&uart_ringbuffer_struct, uart_read_buffer, data_size);

            // ========== 操作区域 ==========
            // 这里可以根据实际需求处理接收到的数据
            // 示例：打印十六进制数据，避免把二进制直接当字符串输出造成栈开销
            ESP_LOGI(TAG, "Received %u bytes", data_size);
            ESP_LOG_BUFFER_HEX_LEVEL(TAG, uart_read_buffer, data_size, ESP_LOG_INFO);
            //uart_printf("Echo: %.*s\r\n", data_size, (char *)uart_read_buffer);
            // ==============================

            // 重置环形缓冲区（类似STM32代码）
            rt_ringbuffer_reset(&uart_ringbuffer_struct);
        }

        // 延时，避免占用过多CPU资源
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * @brief 初始化UART应用（类似STM32的myusart_init）
 */
void uart_app_init(void)
{
    esp_err_t ret;

    /* 1. 配置UART参数 */
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // 安装UART驱动，配置事件队列
    ret = uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE * 2, UART_BUF_SIZE * 2,
                             20, &uart_event_queue, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install UART driver");
        return;
    }

    // 配置UART参数
    ret = uart_param_config(UART_PORT_NUM, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure UART parameters");
        return;
    }

    // 设置UART引脚
    ret = uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN,
                      UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set UART pins");
        return;
    }

    /* 2. 初始化环形缓冲区（类似STM32代码） */
    rt_ringbuffer_init(&uart_ringbuffer_struct, uart_ringbuffer,
                      sizeof(uart_ringbuffer));

    ESP_LOGI(TAG, "Ringbuffer initialized, size: %d bytes", sizeof(uart_ringbuffer));

    /* 3. 创建UART事件处理任务（类似STM32的DMA中断+回调） */
    BaseType_t task_ret = xTaskCreate(uart_event_task, "uart_event_task",
                                     3072, NULL, 12, NULL);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create UART event task");
        return;
    }

    /* 4. 创建UART数据处理任务（类似STM32的uart1_task） */
    task_ret = xTaskCreate(uart_app_task, "uart_app_task", 4096, NULL, 10, NULL);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create UART application task");
        return;
    }

    ESP_LOGI(TAG, "UART initialized successfully on UART%d (TX:%d, RX:%d, Baud:%d)",
            UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_BAUD_RATE);
}

/**
 * @brief 串口发送（类似STM32的uart_printf）
 */
int uart_printf(const char* format, ...)
{
    char buffer[256];  // 设定一个足够大的缓冲区
    va_list args;
    va_start(args, format);

    // 使用vsnprintf进行格式化输出
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (len < 0) {
        ESP_LOGE(TAG, "Format string error");
        return -1;  // 格式化失败
    }

    if (len >= sizeof(buffer)) {
        ESP_LOGW(TAG, "Output truncated, buffer too small");
    }

    // 通过UART发送格式化后的字符串
    int sent = uart_write_bytes(UART_PORT_NUM, buffer, len);

    return sent;
}

/**
 * @brief 获取环形缓冲区数据长度
 */
uint16_t uart_get_data_len(void)
{
    return rt_ringbuffer_data_len(&uart_ringbuffer_struct);
}
