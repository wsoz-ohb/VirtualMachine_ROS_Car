# ROS 小车项目

ESP32-S3 + ROS 双轮差速驱动小车，集成 IMU 姿态传感器、编码器反馈、PID 速度闭环控制。

## 项目结构

```
ros_car/
├── firmware/              # ESP32-S3 固件代码（Windows 开发）
│   ├── main/             # 应用层代码
│   ├── components/       # 驱动层组件
│   ├── CMakeLists.txt
│   └── sdkconfig         # ESP-IDF 配置
│
├── ros_workspace/         # ROS 节点代码（虚拟机开发）
│   ├── src/              # ROS 包源码
│   ├── launch/           # 启动文件
│   └── config/           # 参数配置
│
└── tools/                 # 辅助工具脚本
```

## 快速开始

### 1. Windows 端 - ESP32 固件开发

```bash
cd firmware/

# 设置目标芯片（首次）
idf.py set-target esp32s3

# 构建
idf.py build

# 烧录并监控（替换 COM3 为实际端口）
idf.py -p COM3 flash monitor
```

详细说明见 [firmware/CLAUDE.md](firmware/CLAUDE.md)

### 2. 虚拟机端 - ROS 节点开发

```bash
# 克隆仓库
git clone <仓库地址>
cd ros_car/ros_workspace

# 初始化 ROS 工作空间
catkin_make  # ROS1
# 或
colcon build  # ROS2

# 添加到环境变量
source devel/setup.bash
```

详细说明见 [ros_workspace/README.md](ros_workspace/README.md)

## 硬件配置

- **主控**: ESP32-S3
- **电机驱动**: TB6612FNG 双 H 桥
- **编码器**: 13 线编码器，减速比 1:20，4 倍频
- **IMU**: BNO08x（I2C 通信）
- **通信**: 串口（ESP32 ↔ ROS）

## 开发工作流

### Windows 修改固件后推送

```bash
cd D:\esp-code\ros_car
git pull                          # 先拉取虚拟机的更新
# 修改 firmware/ 下的文件
git add firmware/
git commit -m "fix: 修复电机 PID 参数"
git push
```

### 虚拟机修改 ROS 代码后推送

```bash
cd ~/workspace/ros_car
git pull                          # 先拉取 Windows 的更新
# 修改 ros_workspace/ 下的文件
git add ros_workspace/
git commit -m "feat: 添加激光雷达驱动节点"
git push
```

## 通信协议

ESP32 通过串口与 ROS 通信：

**ESP32 → ROS（上报数据）**
- 编码器速度（左轮/右轮）
- IMU 姿态（roll/pitch/yaw）
- 电机状态

**ROS → ESP32（控制指令）**
- 目标速度（线速度 + 角速度）
- PID 参数调整

## 技术栈

- **固件**: ESP-IDF v5.1.2, FreeRTOS
- **ROS**: ROS Noetic (Ubuntu 20.04) 或 ROS2 Humble
- **版本控制**: Git (GitHub/Gitee)

## 许可证

[根据实际情况填写]
