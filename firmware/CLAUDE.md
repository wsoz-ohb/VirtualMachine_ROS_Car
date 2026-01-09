# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP32-S3 ROS 小车项目，基于 ESP-IDF v5.1.2 框架。实现双轮差速驱动小车，集成 IMU 姿态传感器、编码器反馈、PID 速度闭环控制。

## Development Environment

- ESP-IDF: v5.1.2 (`d:\esp32-idf-ahy\5.1.2\frameworks\esp-idf-v5.1.2`)
- Python: `d:\esp32-idf-ahy\5.1.2\python_env\idf5.1_py3.11_env\Scripts\python.exe`
- Target: ESP32-S3

## Build Commands

```bash
# 设置目标芯片（首次或切换环境时执行）
idf.py set-target esp32s3

# 构建
idf.py build

# 烧录并监控（替换 COMx 为实际端口）
idf.py -p COMx flash monitor

# 清理构建缓存
idf.py fullclean

# 配置菜单（调整引脚、日志级别等）
idf.py menuconfig
```

若 idf.py 不在 PATH 中，使用完整路径：
```bash
d:\esp32-idf-ahy\5.1.2\python_env\idf5.1_py3.11_env\Scripts\python.exe d:\esp32-idf-ahy\5.1.2\frameworks\esp-idf-v5.1.2\tools\idf.py build
```

## Architecture

### 分层结构

```
main/
├── main.c              # 入口，初始化各 app 模块并启动主循环
└── myapp/              # 应用层（高层业务逻辑）
    ├── motor_app       # 电机控制应用（PID 速度环、编码器采样）
    ├── bno08x_app      # IMU 姿态应用（欧拉角解算）
    ├── key_app         # 按键应用
    └── uart_app        # 串口通信应用

components/             # 驱动层（可复用硬件抽象）
├── tb6612_motor/       # TB6612 双 H 桥电机驱动（LEDC PWM）
├── my_encoder/         # 编码器驱动（PCNT 脉冲计数）
├── my_pid/             # PID 控制器（位置式/增量式）
├── my_bno08x/          # BNO08x IMU 驱动（I2C + SHTP 协议）
├── my_oled/            # OLED 显示驱动
├── key/                # 按键驱动
└── ringbuffer/         # 环形缓冲区
```

### 数据流

```
编码器(PCNT) → motor_encoder → motor_app(PID计算) → tb6612_motor(PWM输出)
                                    ↑
BNO08x(I2C) → bno08x_app → 姿态数据(roll/pitch/yaw)
```

### 关键组件 API

**tb6612_motor** - 电机驱动
- `tb6612_gpio_register()` - 注册 GPIO 引脚
- `tb6612_motor_pwm_set()` - 设置 PWM 占空比（正负值控制方向）

**my_encoder** - 编码器
- `motor_encoder_init()` - 初始化 PCNT 单元
- `motor_encoder_get_speed_pulse()` - 获取速度脉冲
- `motor_encoder_speed_update_callback()` - 50ms 定时回调

**my_pid** - PID 控制
- `PID_Init()` - 初始化参数
- `PID_location()` - 位置式 PID
- `PID_increment()` - 增量式 PID

**my_bno08x** - IMU
- `BNO080_Init()` - 初始化 I2C 通信
- `enableGameRotationVector()` - 启用姿态报告
- `QuaternionToEulerAngles()` - 四元数转欧拉角

## Coding Conventions

- 遵循 ESP-IDF C 风格：4 空格缩进，函数大括号换行，`snake_case` 命名
- 头文件放 `components/<name>/inc/`，实现放 `src/`
- 使用 `ESP_LOGx(TAG, ...)` 日志宏，TAG 大写定义在文件顶部
- GPIO 常量使用描述性命名（如 `TB6612_IN1_GPIO`）

## Hardware Configuration

### 电机驱动 (TB6612)
- 左电机: PWM=GPIO4, IN1=GPIO35, IN2=GPIO36
- 右电机: PWM=GPIO5, IN1=GPIO47, IN2=GPIO48

### 编码器
- 左编码器: A=GPIO6, B=GPIO7 (PCNT Unit 0)
- 右编码器: A=GPIO15, B=GPIO16 (PCNT Unit 1)
- 参数: 13线编码器, 减速比20, 4倍频

### IMU (BNO08x)
- I2C地址: 0x4B
- 通信协议: SHTP over I2C
- 输出: 四元数 → 欧拉角 (roll/pitch/yaw)

## PID Parameters

当前电机速度环 PID 参数（在 `motor_app.c:18-19`）:
- Kp = 6.2
- Ki = 0.62
- Kd = 0.0
- 输出范围: [-1023, 1023]
