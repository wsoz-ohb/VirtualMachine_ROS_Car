#ifndef MOTOR_ENCODER_H
#define MOTOR_ENCODER_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/pulse_cnt.h"
#include "esp_timer.h"
#include "esp_err.h"

// 编码器事件结构体
typedef struct {
    uint32_t timestamp;
    int direction;                  // 方向: 1 为正转，-1 为反转
} encoder_event_t;

typedef struct {
    uint8_t encoder_a_gpio;
    uint8_t encoder_b_gpio;
    pcnt_unit_handle_t pcnt_unit;   // PCNT 单元句柄
    int unit_id;                    // 单元 ID（0 或 1）

    // 13 线编码器参数
    float ppr;                      // 每圈脉冲数（13）
    float gear_ratio;               // 减速比（20）
    float multiply_factor;          // 倍频数（4）

    // 计数相关
    volatile int32_t total_pulse;   // 累积总脉冲
    volatile int16_t speed_pulse;   // 当前速度脉冲（每个采样周期）
    volatile float rpm;             // 当前车轮 RPM（带正负号）
    int last_count;                 // 上一次计数值
    bool is_initialized;            // 是否已初始化
} motor_encoder_t;

// 函数声明
esp_err_t motor_encoder_init(motor_encoder_t* encoder, int unit_id,
                             uint8_t gpio_a, uint8_t gpio_b);
esp_err_t motor_encoder_deinit(motor_encoder_t* encoder);
esp_err_t motor_encoder_reset(motor_encoder_t* encoder);
int16_t motor_encoder_get_speed_pulse(motor_encoder_t* encoder);    // 获取当前速度脉冲
int32_t motor_encoder_get_total_pulse(motor_encoder_t* encoder);    // 获取总脉冲数
float motor_encoder_get_circle(motor_encoder_t* encoder);           // 获取转过的圈数
void motor_encoder_speed_update_callback(void* arg);                // 50ms 定时回调函数

#endif // MOTOR_ENCODER_H
