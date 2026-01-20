#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
编码器/里程计 UDP 接收节点
功能：接收 ESP32 通过 UDP 发送的编码器数据（JSON 格式），计算里程计
端口：8082
数据格式：{
  "type": "odometry",
  "left_rpm": 45.3,    // 左轮转速 (RPM)
  "right_rpm": -42.8   // 右轮转速 (RPM)，负值表示反向
}
发布话题：
  - /odom (nav_msgs/Odometry) - 里程计数据
  - /wheel_speeds (geometry_msgs/TwistStamped) - 轮速数据
"""

import rospy
import socket
import json
import math
import tf
from nav_msgs.msg import Odometry
from geometry_msgs.msg import Quaternion, Twist, TwistStamped, Point, Pose, PoseWithCovariance, TwistWithCovariance
from std_msgs.msg import Header


class OdomReceiver:
    def __init__(self):
        # 初始化 ROS 节点
        rospy.init_node('odom_receiver', anonymous=False)

        # 获取参数
        self.udp_ip = rospy.get_param('~udp_ip', '0.0.0.0')
        self.udp_port = rospy.get_param('~udp_port', 8082)
        self.frame_id = rospy.get_param('~frame_id', 'odom')
        self.child_frame_id = rospy.get_param('~child_frame_id', 'base_link')

        # 小车物理参数（需要根据实际小车调整）
        self.wheel_radius = rospy.get_param('~wheel_radius', 0.033)  # 轮子半径 (m)，默认33mm
        self.wheel_base = rospy.get_param('~wheel_base', 0.15)      # 轮距 (m)，默认150mm
        self.publish_tf = rospy.get_param('~publish_tf', True)       # 是否发布 TF

        # 里程计状态
        self.x = 0.0
        self.y = 0.0
        self.theta = 0.0
        self.last_time = None

        # 创建发布者
        self.odom_pub = rospy.Publisher('/odom', Odometry, queue_size=10)
        self.wheel_pub = rospy.Publisher('/wheel_speeds', TwistStamped, queue_size=10)

        # TF 广播器
        if self.publish_tf:
            self.tf_broadcaster = tf.TransformBroadcaster()

        # 创建 UDP socket
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind((self.udp_ip, self.udp_port))
        self.sock.settimeout(1.0)

        rospy.loginfo("里程计 UDP 接收节点已启动")
        rospy.loginfo("监听地址: %s:%d" % (self.udp_ip, self.udp_port))
        rospy.loginfo("轮子半径: %.3f m, 轮距: %.3f m" % (self.wheel_radius, self.wheel_base))
        rospy.loginfo("发布话题: /odom, /wheel_speeds")

    def rpm_to_velocity(self, rpm):
        """
        将 RPM 转换为线速度 (m/s)
        v = RPM * 2 * pi * r / 60
        """
        return rpm * 2.0 * math.pi * self.wheel_radius / 60.0

    def compute_odometry(self, left_rpm, right_rpm, current_time):
        """
        根据轮速计算里程计
        差速驱动运动学模型
        """
        # 计算左右轮线速度
        v_left = self.rpm_to_velocity(left_rpm)
        v_right = self.rpm_to_velocity(right_rpm)

        # 计算线速度和角速度
        linear_vel = (v_left + v_right) / 2.0
        angular_vel = (v_right - v_left) / self.wheel_base

        # 计算时间差
        if self.last_time is None:
            self.last_time = current_time
            return linear_vel, angular_vel

        dt = (current_time - self.last_time).to_sec()
        self.last_time = current_time

        # 防止异常大的 dt
        if dt <= 0 or dt > 1.0:
            return linear_vel, angular_vel

        # 更新位姿（使用中点积分）
        delta_theta = angular_vel * dt
        delta_x = linear_vel * math.cos(self.theta + delta_theta / 2.0) * dt
        delta_y = linear_vel * math.sin(self.theta + delta_theta / 2.0) * dt

        self.x += delta_x
        self.y += delta_y
        self.theta += delta_theta

        # 归一化角度到 [-pi, pi]
        self.theta = math.atan2(math.sin(self.theta), math.cos(self.theta))

        return linear_vel, angular_vel

    def parse_and_publish(self, data_str):
        """
        解析 JSON 数据并发布里程计消息
        """
        try:
            json_data = json.loads(data_str)

            if json_data.get('type') != 'odometry':
                rospy.logwarn("接收到非里程计数据: %s" % json_data.get('type'))
                return

            left_rpm = json_data.get('left_rpm', 0.0)
            right_rpm = json_data.get('right_rpm', 0.0)

            current_time = rospy.Time.now()

            # 计算里程计
            linear_vel, angular_vel = self.compute_odometry(left_rpm, right_rpm, current_time)

            # 创建四元数
            odom_quat = tf.transformations.quaternion_from_euler(0, 0, self.theta)

            # 发布 TF
            if self.publish_tf:
                self.tf_broadcaster.sendTransform(
                    (self.x, self.y, 0.0),
                    odom_quat,
                    current_time,
                    self.child_frame_id,
                    self.frame_id
                )

            # 创建 Odometry 消息
            odom_msg = Odometry()
            odom_msg.header.stamp = current_time
            odom_msg.header.frame_id = self.frame_id
            odom_msg.child_frame_id = self.child_frame_id

            # 位置
            odom_msg.pose.pose.position.x = self.x
            odom_msg.pose.pose.position.y = self.y
            odom_msg.pose.pose.position.z = 0.0
            odom_msg.pose.pose.orientation.x = odom_quat[0]
            odom_msg.pose.pose.orientation.y = odom_quat[1]
            odom_msg.pose.pose.orientation.z = odom_quat[2]
            odom_msg.pose.pose.orientation.w = odom_quat[3]

            # 位置协方差（位置误差随距离累积）
            odom_msg.pose.covariance = [
                0.01, 0, 0, 0, 0, 0,
                0, 0.01, 0, 0, 0, 0,
                0, 0, 1e6, 0, 0, 0,  # z 方向不可观测
                0, 0, 0, 1e6, 0, 0,
                0, 0, 0, 0, 1e6, 0,
                0, 0, 0, 0, 0, 0.03
            ]

            # 速度
            odom_msg.twist.twist.linear.x = linear_vel
            odom_msg.twist.twist.linear.y = 0.0
            odom_msg.twist.twist.linear.z = 0.0
            odom_msg.twist.twist.angular.x = 0.0
            odom_msg.twist.twist.angular.y = 0.0
            odom_msg.twist.twist.angular.z = angular_vel

            # 速度协方差
            odom_msg.twist.covariance = [
                0.01, 0, 0, 0, 0, 0,
                0, 1e6, 0, 0, 0, 0,  # y 方向速度不可观测
                0, 0, 1e6, 0, 0, 0,
                0, 0, 0, 1e6, 0, 0,
                0, 0, 0, 0, 1e6, 0,
                0, 0, 0, 0, 0, 0.02
            ]

            self.odom_pub.publish(odom_msg)

            # 发布轮速数据
            wheel_msg = TwistStamped()
            wheel_msg.header.stamp = current_time
            wheel_msg.header.frame_id = self.child_frame_id
            wheel_msg.twist.linear.x = self.rpm_to_velocity(left_rpm)   # 左轮速度
            wheel_msg.twist.linear.y = self.rpm_to_velocity(right_rpm)  # 右轮速度
            wheel_msg.twist.angular.x = left_rpm   # 左轮 RPM (供调试)
            wheel_msg.twist.angular.y = right_rpm  # 右轮 RPM (供调试)
            self.wheel_pub.publish(wheel_msg)

            rospy.logdebug("里程计: x=%.3f y=%.3f theta=%.3f v=%.3f w=%.3f" %
                          (self.x, self.y, self.theta, linear_vel, angular_vel))

        except json.JSONDecodeError as e:
            rospy.logerr("JSON 解析失败: %s" % str(e))
        except Exception as e:
            rospy.logerr("处理数据时出错: %s" % str(e))

    def run(self):
        """
        主循环
        """
        rospy.loginfo("开始接收编码器数据...")

        while not rospy.is_shutdown():
            try:
                data, addr = self.sock.recvfrom(1024)
                data_str = data.decode('utf-8')
                self.parse_and_publish(data_str)
            except socket.timeout:
                continue
            except Exception as e:
                if not rospy.is_shutdown():
                    rospy.logerr("接收数据时出错: %s" % str(e))

        self.sock.close()
        rospy.loginfo("里程计 UDP 接收节点已关闭")


def main():
    try:
        receiver = OdomReceiver()
        receiver.run()
    except rospy.ROSInterruptException:
        pass


if __name__ == '__main__':
    main()
