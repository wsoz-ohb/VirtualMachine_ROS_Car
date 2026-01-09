# ROS Workspace

此目录用于存放 ROS 节点代码，与 ESP32 固件通过串口通信。

## 目录结构

```
ros_workspace/
├── src/                    # ROS 包源码
│   ├── car_driver/        # 串口通信驱动节点
│   ├── car_control/       # 运动控制节点
│   └── car_description/   # URDF 机器人模型
├── launch/                 # 启动文件
└── config/                 # 参数配置文件
```

## 开发指南

### 在虚拟机中克隆仓库后

```bash
cd ~/workspace
git clone <仓库地址>
cd ros_car/ros_workspace

# 初始化 ROS 工作空间（首次）
catkin_make  # 或 colcon build（ROS2）

# 添加到环境变量
source devel/setup.bash
```

### 创建新的 ROS 包

```bash
cd src/
catkin_create_pkg car_driver rospy std_msgs geometry_msgs
```

## 通信协议

ESP32 固件通过串口发送以下数据：
- 编码器速度
- IMU 姿态（roll/pitch/yaw）
- 电机状态

ROS 节点通过串口发送控制指令：
- 目标速度（线速度 + 角速度）
- PID 参数调整

详细协议见 `firmware/CLAUDE.md`
