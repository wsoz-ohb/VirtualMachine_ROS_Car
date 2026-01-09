# ESP32 MCPWM编码器实现指南

本文档详细介绍如何使用ESP32的MCPWM模块实现编码器功能，以及与STM32编码器实现的对比。

## 目录
1. [MCPWM模块概述](#mcpwm模块概述)
2. [MCPWM编码器原理](#mcpwm编码器原理)
3. [具体实现步骤](#具体实现步骤)
4. [与STM32的对比](#与stm32的对比)
5. [关键代码解析](#关键代码解析)
6. [常见问题](#常见问题)

## MCPWM模块概述

### 什么是MCPWM？
MCPWM (Motor Control Pulse Width Modulation) 是ESP32专门为电机控制设计的硬件模块。每个ESP32有2个MCPWM单元：
- **MCPWM_UNIT_0**：包含3个定时器(TIMER_0, TIMER_1, TIMER_2)
- **MCPWM_UNIT_1**：包含3个定时器(TIMER_0, TIMER_1, TIMER_2)

### MCPWM的主要功能
1. **PWM生成**：生成电机控制PWM信号
2. **信号捕获**：捕获外部信号的边沿和时间
3. **编码器接口**：专门的编码器信号处理功能
4. **故障保护**：硬件级别的故障检测

### 为什么选择MCPWM做编码器？
- **硬件支持**：专门的编码器信号处理
- **高精度**：硬件计数，不占用CPU时间
- **多通道**：支持多个编码器同时工作
- **中断处理**：硬件中断，响应速度快

## MCPWM编码器原理

### 编码器信号类型
编码器通常输出两路信号：
- **A相**：主脉冲信号
- **B相**：相位差90°的脉冲信号

### 方向判断原理
```
正转时：A相上升沿时B相为低电平，A相下降沿时B相为高电平
反转时：A相上升沿时B相为高电平，A相下降沿时B相为低电平
```

### MCPWM编码器工作模式
MCPWM支持两种编码器工作模式：

1. **计数模式**：使用CAP0和CAP1捕获A相和B相信号
2. **正交编码器模式**：自动处理A/B相信号的正交关系

## 具体实现步骤

### 步骤1：配置MCPWM基本参数
```c
mcpwm_config_t pwm_config = {
    .frequency = 1000,                    // 频率（编码器模式下不重要）
    .duty_mode = MCPWM_DUTY_MODE_0,       // 占空比模式
    .counter_mode = MCPWM_UP_DOWN_COUNTER, // 上下计数，适合编码器
};
mcpwm_init(unit, timer, &pwm_config);
```

**关键点**：
- `MCPWM_UP_DOWN_COUNTER`：上下计数模式，可以处理正反方向
- 频率在编码器模式下不直接影响功能

### 步骤2：配置GPIO输入
```c
// 配置A相和B相GPIO为输入模式
gpio_config_t gpio_conf = {
    .intr_type = GPIO_INTR_DISABLE,      // 禁用GPIO中断
    .mode = GPIO_MODE_INPUT,             // 输入模式
    .pin_bit_mask = (1ULL << gpio_a) | (1ULL << gpio_b),
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .pull_up_en = GPIO_PULLUP_ENABLE      // 启用上拉电阻
};
gpio_config(&gpio_conf);

// 将GPIO连接到MCPWM捕获通道
mcpwm_gpio_init(unit, MCPWM_CAP_0, gpio_a);  // A相 -> CAP0
mcpwm_gpio_init(unit, MCPWM_CAP_1, gpio_b);  // B相 -> CAP1
```

**关键点**：
- A相连接到CAP0，B相连接到CAP1
- 启用上拉电阻防止信号浮空

### 步骤3：设置计数器初始值
```c
// 设置计数器到中心点32767 (0x7FFF)
mcpwm_counter_set_value(unit, timer, 0x7FFF);
```

**为什么是0x7FFF？**
- 16位计数器的范围是0-65535
- 0x7FFF = 32767，正好是中间值
- 这样可以向正负两个方向计数，避免溢出

### 步骤4：创建50ms定时器
```c
esp_timer_create_args_t timer_args = {
    .callback = motor_encoder_speed_update_callback,
    .name = "encoder_speed_timer"
};
esp_timer_create(&timer_args, &g_speed_timer);
esp_timer_start_periodic(g_speed_timer, 50000); // 50ms
```

### 步骤5：实现速度计算
```c
static int16_t calculate_speed_pulse(motor_encoder_t* encoder)
{
    int16_t speed_pulse;
    uint16_t current_count;

    // 读取当前计数值
    current_count = mcpwm_capture_signal_get_value(encoder->mcpwm_unit,
                                                  encoder->timer_num,
                                                  MCPWM_SELECT_CAP0);

    // 重置计数器到中心点
    mcpwm_counter_set_value(encoder->mcpwm_unit,
                           encoder->timer_num,
                           encoder->center_point);

    // 计算与中心点的差值
    speed_pulse = (int16_t)(current_count - encoder->center_point);

    return speed_pulse;
}
```

**核心算法**：
1. 读取当前计数器值
2. 重置计数器回中心点
3. 计算差值得到这50ms内的脉冲数
4. 正值表示正转，负值表示反转

## 与STM32的对比

### STM32实现方式
```c
// STM32使用硬件编码器定时器
HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
__HAL_TIM_SetCounter(&htim2, 0x7FFF);

// 读取和重置
current_count = (uint16_t)__HAL_TIM_GetCounter(&htim2);
__HAL_TIM_SetCounter(&htim2, 0x7FFF);
speed_pulse = (int16_t)(current_count - 0x7FFF);
```

### ESP32实现方式
```c
// ESP32使用MCPWM捕获模式
mcpwm_gpio_init(unit, MCPWM_CAP_0, gpio_a);
mcpwm_counter_set_value(unit, timer, 0x7FFF);

// 读取和重置
current_count = mcpwm_capture_signal_get_value(unit, timer, MCPWM_SELECT_CAP0);
mcpwm_counter_set_value(unit, timer, 0x7FFF);
speed_pulse = (int16_t)(current_count - 0x7FFF);
```

### 主要区别
| 特性 | STM32 | ESP32 |
|------|-------|-------|
| 硬件模块 | TIM定时器 | MCPWM单元 |
| 编码器模式 | 专用编码器模式 | 捕获模式 |
| API函数 | HAL_TIM_xxx | mcpwm_xxx |
| 定时器来源 | 专用硬件定时器 | esp_timer |

## 关键代码解析

### 1. 结构体设计
```c
typedef struct{
    uint8_t encoder_a_gpio;              // A相GPIO
    uint8_t encoder_b_gpio;              // B相GPIO
    mcpwm_unit_t mcpwm_unit;             // MCPWM单元
    mcpwm_timer_t timer_num;             // 定时器编号

    // 13线编码器参数
    float ppr;                           // 编码器每转脉冲数 (13)
    float gear_ratio;                    // 减速比 (20)
    float multiply_factor;               // 倍频数 (4)

    // 计数相关
    volatile int32_t total_pulse;        // 总脉冲累计
    volatile int16_t speed_pulse;        // 当前速度脉冲
    uint16_t center_point;               // 计数器中心点 (0x7FFF)
    bool is_initialized;                 // 初始化标志
}motor_encoder_t;
```

### 2. 定时器回调函数
```c
void motor_encoder_speed_update_callback(void* arg)
{
    // 更新左轮编码器速度
    if (g_encoders[0] != NULL) {
        g_encoders[0]->speed_pulse = calculate_speed_pulse(g_encoders[0]);
        g_encoders[0]->total_pulse += g_encoders[0]->speed_pulse;
    }

    // 更新右轮编码器速度
    if (g_encoders[1] != NULL) {
        g_encoders[1]->speed_pulse = calculate_speed_pulse(g_encoders[1]);
        g_encoders[1]->total_pulse += g_encoders[1]->speed_pulse;
    }
}
```

**作用**：
- 每50ms调用一次
- 计算当前速度脉冲
- 累加到总脉冲数

### 3. 圈数计算
```c
float motor_encoder_get_circle(motor_encoder_t* encoder)
{
    // 公式: 总脉冲数 / (4倍频 * 减速比 * PPR)
    return (float)encoder->total_pulse / (encoder->multiply_factor *
                                          encoder->gear_ratio *
                                          encoder->ppr);
}
```

**参数说明**：
- 4倍频：检测A相和B相的所有边沿
- 减速比20：电机到车轮的减速比
- PPR=13：编码器线数

## 常见问题

### Q1: 为什么不使用MCPWM的正交编码器模式？
**A**: ESP32的正交编码器模式在某些版本中可能不稳定。使用捕获模式更可靠，而且与STM32的逻辑一致。

### Q2: 中心点为什么要设置为0x7FFF？
**A**:
- 防止计数器溢出
- 可以检测正反两个方向的运动
- 与STM32的设置保持一致

### Q3: 50ms采样周期是如何确定的？
**A**:
- 这是你的STM32代码中使用的采样周期
- 50ms = 20Hz，足够获取平滑的速度数据
- 不会过度占用CPU资源

### Q4: 如何处理编码器信号噪声？
**A**:
- GPIO启用上拉电阻
- 在硬件上添加滤波电容
- 软件上可以添加简单的滤波算法

### Q5: 支持多少个编码器？
**A**:
- ESP32有2个MCPWM单元
- 每个单元可以处理多个编码器（通过不同的定时器）
- 当前实现支持2个编码器（左轮和右轮）

## 使用示例

完整的初始化和使用示例：

```c
#include "motor_encoder.h"

motor_encoder_t left_encoder;
motor_encoder_t right_encoder;

void app_main(void)
{
    // 初始化左轮编码器
    motor_encoder_init(&left_encoder,
                      0,               // 单元ID 0
                      25,              // A相GPIO
                      26);             // B相GPIO

    // 初始化右轮编码器
    motor_encoder_init(&right_encoder,
                      1,               // 单元ID 1
                      33,              // A相GPIO
                      32);             // B相GPIO

    while(1) {
        // 读取编码器数据
        int16_t left_speed = motor_encoder_get_speed_pulse(&left_encoder);
        int16_t right_speed = motor_encoder_get_speed_pulse(&right_encoder);

        int32_t left_total = motor_encoder_get_total_pulse(&left_encoder);
        int32_t right_total = motor_encoder_get_total_pulse(&right_encoder);

        float left_circle = motor_encoder_get_circle(&left_encoder);
        float right_circle = motor_encoder_get_circle(&right_encoder);

        printf("Left: Speed=%d, Total=%ld, Circle=%.3f\n",
               left_speed, left_total, left_circle);
        printf("Right: Speed=%d, Total=%ld, Circle=%.3f\n",
               right_speed, right_total, right_circle);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

## 总结

ESP32的MCPWM模块为实现编码器功能提供了强大的硬件支持。通过模仿STM32的中心点重置算法，我们实现了：
- 高精度的编码器计数
- 稳定的速度测量
- 与STM32代码一致的接口
- 简单可靠的实现方式

这种方法既保持了硬件级的高性能，又与你的STM32经验保持一致，便于开发和调试。