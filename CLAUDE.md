# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP32-S3 + ROS 双轮差速驱动小车项目。固件基于 ESP-IDF v5.1.2，ROS 工作空间基于 ROS Noetic。集成 BNO08x IMU 姿态传感器、编码器反馈、PID 速度闭环控制，通过 WiFi UDP 和串口与 ROS 通信。

## Repository Structure

```
firmware/              # ESP32-S3 固件代码 (Windows 开发)
├── main/myapp/        # 应用层: motor_app, bno08x_app, uart_app, wifi_app, socket_app
└── components/        # 驱动层: tb6612_motor, my_encoder, my_pid, my_bno08x, ringbuffer

ros_workspace/         # ROS 工作空间 (虚拟机开发)
└── src/ros_udp/       # IMU UDP 接收节点

tools/                 # YDLidar SDK 和开发文档
```

## Build Commands

### ESP32 固件 (Windows)

```bash
cd firmware/
idf.py set-target esp32s3    # 首次设置目标芯片
idf.py build                 # 编译
idf.py -p COMx flash monitor # 烧录并监控
idf.py fullclean             # 清理构建缓存
idf.py menuconfig            # 配置菜单
```

### ROS 工作空间 (虚拟机)

```bash
cd ros_workspace/
catkin_make                  # 构建
source devel/setup.bash      # 加载环境
rosrun ros_udp imu_receiver.py  # 运行 IMU 接收节点
```

## Architecture

### 固件分层

```
应用层 (main/myapp/)
├── motor_app      电机 PID 速度闭环控制
├── bno08x_app     IMU 姿态解算 (四元数/欧拉角/角速度/加速度)
├── my_wifi_app    WiFi 连接管理
├── my_socket      UDP 数据发送
└── uart_app       串口通信

驱动层 (components/)
├── tb6612_motor   TB6612 双 H 桥 PWM 驱动
├── my_encoder     PCNT 脉冲计数器编码器驱动
├── my_pid         位置式/增量式 PID 控制器
├── my_bno08x      BNO08x I2C + SHTP 协议驱动
└── ringbuffer     环形缓冲区
```

### 数据流

```
编码器(PCNT) → motor_encoder → motor_app(PID) → tb6612_motor(PWM) → 电机
BNO08x(I2C) → bno08x_app → my_socket(UDP:8080) → ROS imu_receiver → /imu/data
```

### ESP32 → ROS 通信

UDP 端口 8080，JSON 格式：
```json
{
  "type": "imu_data",
  "data": {
    "quat_i", "quat_j", "quat_k", "quat_real",  // 四元数
    "acc_x", "acc_y", "acc_z",                   // 线加速度 (m/s²)
    "gyro_x", "gyro_y", "gyro_z"                 // 角速度 (rad/s)
  }
}
```

ROS 节点 `imu_receiver.py` 解析后发布到话题 `/imu/data` (sensor_msgs/Imu)。

## Hardware Pin Configuration

| 组件 | 引脚配置 |
|------|----------|
| 左电机 | PWM=GPIO4, IN1=GPIO35, IN2=GPIO36 |
| 右电机 | PWM=GPIO5, IN1=GPIO47, IN2=GPIO48 |
| 左编码器 | A=GPIO6, B=GPIO7 (PCNT Unit 0) |
| 右编码器 | A=GPIO15, B=GPIO16 (PCNT Unit 1) |
| IMU | I2C 地址 0x4B |
| UART1 | TX=GPIO17, RX=GPIO18 |

编码器参数：13线，减速比1:20，4倍频

## Key Parameters

PID 参数 (`motor_app.c`):
- Kp=6.2, Ki=0.62, Kd=0.0
- 输出范围: [-1023, 1023]
- 采样周期: 50ms

## Coding Conventions

- 固件遵循 ESP-IDF C 风格：4 空格缩进，`snake_case` 命名
- 头文件放 `components/<name>/inc/`，实现放 `src/`
- 使用 `ESP_LOGx(TAG, ...)` 日志宏，TAG 大写
- GPIO 常量使用描述性命名（如 `TB6612_IN1_GPIO`）
- Commit 使用 Conventional Commits 格式：`feat:`, `fix:` 等

## Development Workflow

Windows 和虚拟机分别开发固件和 ROS 节点，通过 Git 同步：

```bash
# Windows: 修改 firmware/ 后
git pull && git add firmware/ && git commit -m "feat: ..." && git push

# 虚拟机: 修改 ros_workspace/ 后
git pull && git add ros_workspace/ && git commit -m "feat: ..." && git push
```
