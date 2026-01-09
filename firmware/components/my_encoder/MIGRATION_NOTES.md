# 编码器驱动迁移说明

## 迁移概述

已将编码器驱动从**已弃用的 MCPWM 驱动**迁移到**新的 PCNT (脉冲计数器) 驱动**。

## 主要变更

### 1. 头文件更改 (motor_encoder.h)

**之前:**
```c
#include "driver/mcpwm.h"
```

**现在:**
```c
#include "driver/pulse_cnt.h"
#include "esp_timer.h"
```

### 2. 结构体更改

**之前:**
```c
typedef struct{
    mcpwm_unit_t mcpwm_unit;
    mcpwm_timer_t timer_num;
    uint16_t center_point;
    // ...
}motor_encoder_t;
```

**现在:**
```c
typedef struct{
    pcnt_unit_handle_t pcnt_unit;  // PCNT单元句柄
    int unit_id;                    // 单元ID (0或1)
    int last_count;                 // 上次计数值
    // ...
}motor_encoder_t;
```

### 3. API 更改

**初始化函数:**

之前:
```c
motor_encoder_init(&encoder, MCPWM_UNIT_0, MCPWM_TIMER_0, gpio_a, gpio_b);
```

现在:
```c
motor_encoder_init(&encoder, 0, gpio_a, gpio_b);  // 0 或 1 表示单元ID
```

## 技术优势

### 为什么使用 PCNT 而不是 MCPWM？

1. **专用硬件**: PCNT 是专门为脉冲计数设计的外设，更适合编码器应用
2. **4倍频支持**: PCNT 原生支持正交编码器的4倍频模式
3. **方向检测**: 自动检测旋转方向（正转/反转）
4. **低CPU开销**: 硬件计数，不需要CPU干预
5. **官方推荐**: ESP-IDF 官方推荐使用 PCNT 处理编码器信号

### PCNT 配置说明

```c
// 创建PCNT单元，设置计数范围
pcnt_unit_config_t unit_config = {
    .high_limit = 32767,   // 最大计数值
    .low_limit = -32768,   // 最小计数值
};

// 配置两个通道实现4倍频
// 通道A: 监测A相边沿，B相作为电平信号
// 通道B: 监测B相边沿，A相作为电平信号
```

## 使用示例

```c
#include "motor_encoder.h"

motor_encoder_t left_encoder;
motor_encoder_t right_encoder;

void app_main(void)
{
    // 初始化左轮编码器 (单元0)
    motor_encoder_init(&left_encoder, 0, 25, 26);
    
    // 初始化右轮编码器 (单元1)
    motor_encoder_init(&right_encoder, 1, 33, 32);
    
    while(1) {
        // 获取速度脉冲 (每50ms更新一次)
        int16_t left_speed = motor_encoder_get_speed_pulse(&left_encoder);
        int16_t right_speed = motor_encoder_get_speed_pulse(&right_encoder);
        
        // 获取总脉冲数
        int32_t left_total = motor_encoder_get_total_pulse(&left_encoder);
        
        // 获取转过的圈数
        float circles = motor_encoder_get_circle(&left_encoder);
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

## 注意事项

1. ESP32-S3 支持最多 4 个 PCNT 单元，本项目使用 2 个（左右轮）
2. 速度采样周期为 50ms，由 esp_timer 定时器触发
3. 计数范围: -32768 到 32767，超出会自动回绕
4. 支持13线编码器，减速比20:1，4倍频模式

## 编译验证

代码已通过语法检查，无编译错误和警告。
