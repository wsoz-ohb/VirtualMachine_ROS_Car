#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
方向键遥控节点
使用键盘方向键控制小车
  ↑ 前进
  ↓ 后退
  ← 左转
  → 右转
  空格 停止
"""

import rospy
from geometry_msgs.msg import Twist
import sys
import termios
import tty
import select

# 方向键的转义序列
KEY_UP = '\x1b[A'
KEY_DOWN = '\x1b[B'
KEY_RIGHT = '\x1b[C'
KEY_LEFT = '\x1b[D'

class ArrowKeyTeleop:
    def __init__(self):
        rospy.init_node('arrow_key_teleop', anonymous=False)

        # 速度参数
        self.linear_speed = rospy.get_param('~linear_speed', 0.2)    # m/s (约80 RPM)
        self.angular_speed = rospy.get_param('~angular_speed', 0.8)  # rad/s

        self.pub = rospy.Publisher('/cmd_vel', Twist, queue_size=1)

        self.settings = termios.tcgetattr(sys.stdin)

        print("=" * 50)
        print("键盘遥控")
        print("=" * 50)
        print("     q 前左    w 前进    e 前右")
        print("     a 左转    s 后退    d 右转")
        print("     z 后左              c 后右")
        print("")
        print("     空格 停止    x 退出")
        print("=" * 50)
        print("线速度: %.2f m/s, 角速度: %.2f rad/s" % (self.linear_speed, self.angular_speed))
        print("=" * 50)

    def get_key(self):
        """读取键盘输入"""
        tty.setraw(sys.stdin.fileno())
        select.select([sys.stdin], [], [], 0.1)
        key = ''
        if select.select([sys.stdin], [], [], 0)[0]:
            key = sys.stdin.read(1)
            # 检查是否是转义序列（方向键）
            if key == '\x1b':
                key += sys.stdin.read(2)
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self.settings)
        return key

    def run(self):
        twist = Twist()

        try:
            while not rospy.is_shutdown():
                key = self.get_key()

                if key == 'w':
                    twist.linear.x = self.linear_speed
                    twist.angular.z = 0.0
                    print("前进")
                elif key == 's':
                    twist.linear.x = -self.linear_speed
                    twist.angular.z = 0.0
                    print("后退")
                elif key == 'a':
                    twist.linear.x = 0.0
                    twist.angular.z = self.angular_speed
                    print("左转")
                elif key == 'd':
                    twist.linear.x = 0.0
                    twist.angular.z = -self.angular_speed
                    print("右转")
                elif key == 'q':
                    twist.linear.x = self.linear_speed
                    twist.angular.z = self.angular_speed
                    print("前进+左转")
                elif key == 'e':
                    twist.linear.x = self.linear_speed
                    twist.angular.z = -self.angular_speed
                    print("前进+右转")
                elif key == 'z':
                    twist.linear.x = -self.linear_speed
                    twist.angular.z = self.angular_speed
                    print("后退+左转")
                elif key == 'c':
                    twist.linear.x = -self.linear_speed
                    twist.angular.z = -self.angular_speed
                    print("后退+右转")
                elif key == ' ':
                    twist.linear.x = 0.0
                    twist.angular.z = 0.0
                    print("停止")
                elif key == 'x' or key == '\x03':  # x 或 Ctrl+C
                    twist.linear.x = 0.0
                    twist.angular.z = 0.0
                    self.pub.publish(twist)
                    print("退出")
                    break
                else:
                    continue

                self.pub.publish(twist)

        except Exception as e:
            print(e)
        finally:
            # 停止小车
            twist.linear.x = 0.0
            twist.angular.z = 0.0
            self.pub.publish(twist)
            termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self.settings)


def main():
    try:
        teleop = ArrowKeyTeleop()
        teleop.run()
    except rospy.ROSInterruptException:
        pass


if __name__ == '__main__':
    main()
