#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
cmd_vel UDP 发送节点
功能：订阅 /cmd_vel 话题，将速度命令通过 UDP 发送给 ESP32
端口：8082 (ESP32 接收端口)
数据格式：{
  "type": "cmd_vel",
  "linear_x": 0.5,      // 线速度 (m/s)
  "angular_z": 0.1      // 角速度 (rad/s)
}
用途：
  - 键盘遥控 (teleop_twist_keyboard)
  - 导航包 (move_base)
  - 任何发布 /cmd_vel 的控制器
"""

import rospy
import socket
import json
from geometry_msgs.msg import Twist


class CmdVelSender:
    def __init__(self):
        # 初始化 ROS 节点
        rospy.init_node('cmd_vel_sender', anonymous=False)

        # 获取参数
        self.esp32_ip = rospy.get_param('~esp32_ip', '192.168.2.18')  # ESP32 IP 地址
        self.esp32_port = rospy.get_param('~esp32_port', 8082)       # ESP32 接收端口
        self.cmd_vel_topic = rospy.get_param('~cmd_vel_topic', '/cmd_vel')

        # 创建 UDP socket
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

        # 订阅 /cmd_vel 话题
        self.cmd_vel_sub = rospy.Subscriber(
            self.cmd_vel_topic,
            Twist,
            self.cmd_vel_callback,
            queue_size=1
        )

        rospy.loginfo("cmd_vel UDP 发送节点已启动")
        rospy.loginfo("目标地址: %s:%d" % (self.esp32_ip, self.esp32_port))
        rospy.loginfo("订阅话题: %s" % self.cmd_vel_topic)

    def cmd_vel_callback(self, msg):
        """
        /cmd_vel 回调函数，将速度命令转换为 JSON 并发送
        """
        try:
            # 构建 JSON 数据
            cmd_data = {
                "type": "cmd_vel",
                "linear_x": msg.linear.x,
                "angular_z": msg.angular.z
            }

            # 转换为 JSON 字符串
            json_str = json.dumps(cmd_data)

            # 通过 UDP 发送
            self.sock.sendto(json_str.encode('utf-8'), (self.esp32_ip, self.esp32_port))

            rospy.logdebug("已发送: linear_x=%.3f, angular_z=%.3f" %
                          (msg.linear.x, msg.angular.z))

        except Exception as e:
            rospy.logerr("发送数据时出错: %s" % str(e))

    def run(self):
        """
        主循环
        """
        rospy.loginfo("等待 /cmd_vel 消息...")
        rospy.spin()

        # 关闭 socket
        self.sock.close()
        rospy.loginfo("cmd_vel UDP 发送节点已关闭")


def main():
    try:
        sender = CmdVelSender()
        sender.run()
    except rospy.ROSInterruptException:
        pass


if __name__ == '__main__':
    main()
