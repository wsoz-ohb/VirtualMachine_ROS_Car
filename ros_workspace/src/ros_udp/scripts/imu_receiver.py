#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
IMU UDP 接收节点
功能：接收 ESP32 通过 UDP 发送的 IMU 数据（JSON 格式）
端口：8080
数据格式：{
  "type": "imu_data",
  "data": {
    "quat_i": 0.123,      // 四元数 x
    "quat_j": -0.054,     // 四元数 y
    "quat_k": 1.570,      // 四元数 z
    "quat_real": 0.123,   // 四元数 w
    "acc_x": -0.054,      // 线加速度 x (m/s²)
    "acc_y": 1.570,       // 线加速度 y
    "acc_z": 0.123,       // 线加速度 z
    "gyro_x": -0.054,     // 角速度 x (rad/s)
    "gyro_y": 1.570,      // 角速度 y
    "gyro_z": 0.123       // 角速度 z
  }
}
注意：
  - ESP32 发送完整的 IMU 数据（四元数 + 角速度 + 线加速度）
  - 所有数据单位均符合 ROS 标准（四元数无单位，角速度 rad/s，加速度 m/s²）
"""

import rospy
import socket
import json
from sensor_msgs.msg import Imu
from std_msgs.msg import Header
from geometry_msgs.msg import Quaternion, Vector3


class IMUReceiver:
    def __init__(self):
        # 初始化 ROS 节点
        rospy.init_node('imu_receiver', anonymous=False)

        # 获取参数
        self.udp_ip = rospy.get_param('~udp_ip', '0.0.0.0')  # 监听所有接口
        self.udp_port = rospy.get_param('~udp_port', 8080)
        self.frame_id = rospy.get_param('~frame_id', 'imu_link')

        # 创建发布者
        self.imu_pub = rospy.Publisher('/imu/data', Imu, queue_size=10)

        # 创建 UDP socket
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind((self.udp_ip, self.udp_port))
        self.sock.settimeout(1.0)  # 设置超时，便于检查 rospy.is_shutdown()

        rospy.loginfo("IMU UDP 接收节点已启动")
        rospy.loginfo("监听地址: %s:%d" % (self.udp_ip, self.udp_port))
        rospy.loginfo("发布话题: /imu/data")


    def parse_and_publish(self, data_str):
        """
        解析 JSON 数据并发布 IMU 消息
        """
        try:
            # 解析 JSON
            json_data = json.loads(data_str)

            # 检查数据类型
            if json_data.get('type') != 'imu_data':
                rospy.logwarn("接收到非 IMU 数据: %s" % json_data.get('type'))
                return

            imu_data = json_data.get('data', {})

            # 创建 Imu 消息
            imu_msg = Imu()

            # 填充消息头
            imu_msg.header.stamp = rospy.Time.now()
            imu_msg.header.frame_id = self.frame_id

            # 四元数姿态数据（ESP32 直接发送四元数）
            imu_msg.orientation.x = imu_data.get('quat_i', 0.0)
            imu_msg.orientation.y = imu_data.get('quat_j', 0.0)
            imu_msg.orientation.z = imu_data.get('quat_k', 0.0)
            imu_msg.orientation.w = imu_data.get('quat_real', 1.0)  # 默认值 1.0 表示无旋转

            # 角速度数据（rad/s）
            imu_msg.angular_velocity.x = imu_data.get('gyro_x', 0.0)
            imu_msg.angular_velocity.y = imu_data.get('gyro_y', 0.0)
            imu_msg.angular_velocity.z = imu_data.get('gyro_z', 0.0)

            # 线加速度数据（m/s²）
            imu_msg.linear_acceleration.x = imu_data.get('acc_x', 0.0)
            imu_msg.linear_acceleration.y = imu_data.get('acc_y', 0.0)
            imu_msg.linear_acceleration.z = imu_data.get('acc_z', 0.0)

            # 协方差矩阵
            # orientation: 有效数据，设置小的协方差（BNO08x 精度较高）
            imu_msg.orientation_covariance = [0.01, 0, 0,
                                              0, 0.01, 0,
                                              0, 0, 0.01]
            # angular_velocity: 有效数据，设置小的协方差
            imu_msg.angular_velocity_covariance = [0.02, 0, 0,
                                                    0, 0.02, 0,
                                                    0, 0, 0.02]
            # linear_acceleration: 有效数据，设置小的协方差
            imu_msg.linear_acceleration_covariance = [0.04, 0, 0,
                                                       0, 0.04, 0,
                                                       0, 0, 0.04]

            # 发布消息
            self.imu_pub.publish(imu_msg)

            rospy.logdebug("已发布 IMU 数据: quat=[%.3f,%.3f,%.3f,%.3f] gyro=[%.3f,%.3f,%.3f] acc=[%.3f,%.3f,%.3f]" %
                          (imu_msg.orientation.x, imu_msg.orientation.y, imu_msg.orientation.z, imu_msg.orientation.w,
                           imu_msg.angular_velocity.x, imu_msg.angular_velocity.y, imu_msg.angular_velocity.z,
                           imu_msg.linear_acceleration.x, imu_msg.linear_acceleration.y, imu_msg.linear_acceleration.z))

        except json.JSONDecodeError as e:
            rospy.logerr("JSON 解析失败: %s" % str(e))
        except Exception as e:
            rospy.logerr("处理数据时出错: %s" % str(e))

    def run(self):
        """
        主循环：接收 UDP 数据并发布
        """
        rospy.loginfo("开始接收 IMU 数据...")

        while not rospy.is_shutdown():
            try:
                # 接收 UDP 数据
                data, addr = self.sock.recvfrom(1024)  # 缓冲区大小 1024 字节

                # 解码并处理
                data_str = data.decode('utf-8')
                self.parse_and_publish(data_str)

            except socket.timeout:
                # 超时是正常的，用于检查 rospy.is_shutdown()
                continue
            except Exception as e:
                if not rospy.is_shutdown():
                    rospy.logerr("接收数据时出错: %s" % str(e))

        # 关闭 socket
        self.sock.close()
        rospy.loginfo("IMU UDP 接收节点已关闭")


def main():
    try:
        receiver = IMUReceiver()
        receiver.run()
    except rospy.ROSInterruptException:
        pass


if __name__ == '__main__':
    main()
