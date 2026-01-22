#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
传感器数据 UDP 接收节点（统一接收 IMU + 编码器）
功能：接收 ESP32 通过 UDP 发送的传感器数据（JSON 格式）
端口：8080
数据格式：{
  "type": "sensor_data",
  "imu": {
    "quat_i": 0.0, "quat_j": 0.0, "quat_k": 0.0, "quat_real": 1.0,
    "acc_x": 0.0, "acc_y": 0.0, "acc_z": 9.8,
    "gyro_x": 0.0, "gyro_y": 0.0, "gyro_z": 0.0
  },
  "odometry": {
    "left_rpm": 0.0,
    "right_rpm": 0.0
  }
}
发布话题：
  - /imu/data (sensor_msgs/Imu)
  - /odom (nav_msgs/Odometry)
  - /wheel_speeds (geometry_msgs/TwistStamped)
"""

import rospy
import socket
import json
import math
import tf
from sensor_msgs.msg import Imu
from nav_msgs.msg import Odometry
from geometry_msgs.msg import TwistStamped


class SensorReceiver:
    def __init__(self):
        rospy.init_node('sensor_receiver', anonymous=False)

        # UDP 参数
        self.udp_ip = rospy.get_param('~udp_ip', '0.0.0.0')
        self.udp_port = rospy.get_param('~udp_port', 8080)

        # 坐标系参数
        self.imu_frame_id = rospy.get_param('~imu_frame_id', 'imu_link')
        self.odom_frame_id = rospy.get_param('~odom_frame_id', 'odom')
        self.base_frame_id = rospy.get_param('~base_frame_id', 'base_link')

        # 小车物理参数
        self.wheel_radius = rospy.get_param('~wheel_radius', 0.024)  # 轮半径 24mm
        self.wheel_base = rospy.get_param('~wheel_base', 0.125)      # 轮距 125mm
        self.publish_tf = rospy.get_param('~publish_tf', True)

        # 里程计状态
        self.x = 0.0
        self.y = 0.0
        self.theta = 0.0
        self.last_time = None

        # 发布者
        self.imu_pub = rospy.Publisher('/imu/data', Imu, queue_size=10)
        self.odom_pub = rospy.Publisher('/odom', Odometry, queue_size=10)
        self.wheel_pub = rospy.Publisher('/wheel_speeds', TwistStamped, queue_size=10)

        # TF 广播器
        if self.publish_tf:
            self.tf_broadcaster = tf.TransformBroadcaster()

        # UDP socket
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind((self.udp_ip, self.udp_port))
        self.sock.settimeout(1.0)

        rospy.loginfo("传感器 UDP 接收节点已启动")
        rospy.loginfo("监听地址: %s:%d" % (self.udp_ip, self.udp_port))
        rospy.loginfo("轮半径: %.3f m, 轮距: %.3f m" % (self.wheel_radius, self.wheel_base))

    def rpm_to_velocity(self, rpm):
        """RPM 转线速度 (m/s)"""
        return rpm * 2.0 * math.pi * self.wheel_radius / 60.0

    def publish_imu(self, imu_data, stamp):
        """发布 IMU 数据"""
        msg = Imu()
        msg.header.stamp = stamp
        msg.header.frame_id = self.imu_frame_id

        # 四元数
        msg.orientation.x = imu_data.get('quat_i', 0.0)
        msg.orientation.y = imu_data.get('quat_j', 0.0)
        msg.orientation.z = imu_data.get('quat_k', 0.0)
        msg.orientation.w = imu_data.get('quat_real', 1.0)

        # 角速度 (rad/s)
        msg.angular_velocity.x = imu_data.get('gyro_x', 0.0)
        msg.angular_velocity.y = imu_data.get('gyro_y', 0.0)
        msg.angular_velocity.z = imu_data.get('gyro_z', 0.0)

        # 线加速度 (m/s²)
        msg.linear_acceleration.x = imu_data.get('acc_x', 0.0)
        msg.linear_acceleration.y = imu_data.get('acc_y', 0.0)
        msg.linear_acceleration.z = imu_data.get('acc_z', 0.0)

        # 协方差
        msg.orientation_covariance = [0.01, 0, 0, 0, 0.01, 0, 0, 0, 0.01]
        msg.angular_velocity_covariance = [0.02, 0, 0, 0, 0.02, 0, 0, 0, 0.02]
        msg.linear_acceleration_covariance = [0.04, 0, 0, 0, 0.04, 0, 0, 0, 0.04]

        self.imu_pub.publish(msg)

    def publish_odometry(self, odom_data, stamp):
        """发布里程计数据"""
        left_rpm = odom_data.get('left_rpm', 0.0)
        right_rpm = odom_data.get('right_rpm', 0.0)

        # 计算线速度
        v_left = self.rpm_to_velocity(left_rpm)
        v_right = self.rpm_to_velocity(right_rpm)
        linear_vel = (v_left + v_right) / 2.0
        angular_vel = (v_right - v_left) / self.wheel_base

        # 积分计算位姿
        if self.last_time is not None:
            dt = (stamp - self.last_time).to_sec()
            if 0 < dt < 1.0:
                delta_theta = angular_vel * dt
                delta_x = linear_vel * math.cos(self.theta + delta_theta / 2.0) * dt
                delta_y = linear_vel * math.sin(self.theta + delta_theta / 2.0) * dt
                self.x += delta_x
                self.y += delta_y
                self.theta += delta_theta
                self.theta = math.atan2(math.sin(self.theta), math.cos(self.theta))
        self.last_time = stamp

        # 四元数
        odom_quat = tf.transformations.quaternion_from_euler(0, 0, self.theta)

        # 发布 TF
        if self.publish_tf:
            self.tf_broadcaster.sendTransform(
                (self.x, self.y, 0.0),
                odom_quat,
                stamp,
                self.base_frame_id,
                self.odom_frame_id
            )

        # Odometry 消息
        msg = Odometry()
        msg.header.stamp = stamp
        msg.header.frame_id = self.odom_frame_id
        msg.child_frame_id = self.base_frame_id

        msg.pose.pose.position.x = self.x
        msg.pose.pose.position.y = self.y
        msg.pose.pose.position.z = 0.0
        msg.pose.pose.orientation.x = odom_quat[0]
        msg.pose.pose.orientation.y = odom_quat[1]
        msg.pose.pose.orientation.z = odom_quat[2]
        msg.pose.pose.orientation.w = odom_quat[3]

        msg.pose.covariance = [
            0.01, 0, 0, 0, 0, 0,
            0, 0.01, 0, 0, 0, 0,
            0, 0, 1e6, 0, 0, 0,
            0, 0, 0, 1e6, 0, 0,
            0, 0, 0, 0, 1e6, 0,
            0, 0, 0, 0, 0, 0.03
        ]

        msg.twist.twist.linear.x = linear_vel
        msg.twist.twist.angular.z = angular_vel

        msg.twist.covariance = [
            0.01, 0, 0, 0, 0, 0,
            0, 1e6, 0, 0, 0, 0,
            0, 0, 1e6, 0, 0, 0,
            0, 0, 0, 1e6, 0, 0,
            0, 0, 0, 0, 1e6, 0,
            0, 0, 0, 0, 0, 0.02
        ]

        self.odom_pub.publish(msg)

        # 轮速消息
        wheel_msg = TwistStamped()
        wheel_msg.header.stamp = stamp
        wheel_msg.header.frame_id = self.base_frame_id
        wheel_msg.twist.linear.x = v_left
        wheel_msg.twist.linear.y = v_right
        wheel_msg.twist.angular.x = left_rpm
        wheel_msg.twist.angular.y = right_rpm
        self.wheel_pub.publish(wheel_msg)

    def parse_and_publish(self, data_str):
        """解析 JSON 并发布"""
        try:
            json_data = json.loads(data_str)

            if json_data.get('type') != 'sensor_data':
                rospy.logwarn_throttle(5, "未知数据类型: %s" % json_data.get('type'))
                return

            stamp = rospy.Time.now()

            # 发布 IMU
            imu_data = json_data.get('imu', {})
            if imu_data:
                self.publish_imu(imu_data, stamp)

            # 发布里程计
            odom_data = json_data.get('odometry', {})
            if odom_data:
                self.publish_odometry(odom_data, stamp)

        except json.JSONDecodeError as e:
            rospy.logerr("JSON 解析失败: %s" % str(e))
        except Exception as e:
            rospy.logerr("处理数据出错: %s" % str(e))

    def run(self):
        """主循环"""
        rospy.loginfo("开始接收传感器数据...")

        while not rospy.is_shutdown():
            try:
                data, addr = self.sock.recvfrom(2048)
                self.parse_and_publish(data.decode('utf-8'))
            except socket.timeout:
                continue
            except Exception as e:
                if not rospy.is_shutdown():
                    rospy.logerr("接收数据出错: %s" % str(e))

        self.sock.close()
        rospy.loginfo("传感器接收节点已关闭")


def main():
    try:
        receiver = SensorReceiver()
        receiver.run()
    except rospy.ROSInterruptException:
        pass


if __name__ == '__main__':
    main()
