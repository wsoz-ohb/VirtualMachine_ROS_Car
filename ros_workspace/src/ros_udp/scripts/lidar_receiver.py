#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
YDLidar X2 UDP 接收节点
功能：接收 ESP32 通过 UDP 转发的 YDLidar X2 二进制数据
端口：8081
协议：YDLidar X2 原始数据协议
  - 包头: 0x55AA (小端)
  - CT: 包类型 (bit0=1为起始包)
  - LSN: 采样点数
  - FSA/LSA: 起始/结束角度
  - CS: 校验码
  - Si: 采样数据 (2字节/点)
发布话题：
  - /scan (sensor_msgs/LaserScan) - 激光扫描数据
"""

import rospy
import socket
import struct
import math
from sensor_msgs.msg import LaserScan


class YDLidarReceiver:
    # 包头标识
    HEADER = 0x55AA

    def __init__(self):
        # 初始化 ROS 节点
        rospy.init_node('lidar_receiver', anonymous=False)

        # 获取参数
        self.udp_ip = rospy.get_param('~udp_ip', '0.0.0.0')
        self.udp_port = rospy.get_param('~udp_port', 8081)
        self.frame_id = rospy.get_param('~frame_id', 'laser_link')

        # YDLidar X2 参数
        self.angle_min = rospy.get_param('~angle_min', 0.0)          # 最小角度 (rad)
        self.angle_max = rospy.get_param('~angle_max', 2 * math.pi)  # 最大角度 (rad)
        self.range_min = rospy.get_param('~range_min', 0.1)          # 最小距离 (m)
        self.range_max = rospy.get_param('~range_max', 8.0)          # 最大距离 (m)

        # 扫描数据缓存（用于组装完整一圈扫描）
        self.scan_points = []  # [(angle_rad, distance_m), ...]
        self.scan_frequency = 7.0  # 默认扫描频率 Hz
        self.last_was_start = False

        # 数据缓冲区（处理分包）
        self.buffer = bytearray()

        # 创建发布者
        self.scan_pub = rospy.Publisher('/scan', LaserScan, queue_size=10)

        # 创建 UDP socket
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind((self.udp_ip, self.udp_port))
        self.sock.settimeout(1.0)

        rospy.loginfo("YDLidar UDP 接收节点已启动")
        rospy.loginfo("监听地址: %s:%d" % (self.udp_ip, self.udp_port))
        rospy.loginfo("发布话题: /scan")

    def angle_correct(self, distance_mm):
        """
        角度修正（二级解析）
        YDLidar X2 的光学系统导致的角度偏差修正
        """
        if distance_mm == 0:
            return 0.0
        # 修正公式: AngCorrect = atan(21.8 * (155.3 - d) / (155.3 * d))
        d = distance_mm
        try:
            correction = math.atan(21.8 * (155.3 - d) / (155.3 * d))
            return math.degrees(correction)
        except:
            return 0.0

    def parse_packet(self, data):
        """
        解析 YDLidar 数据包
        返回: [(angle_deg, distance_m), ...] 或 None
        """
        if len(data) < 10:  # 最小包长度
            return None

        # 查找包头 0x55AA (小端存储为 AA 55)
        header_idx = -1
        for i in range(len(data) - 1):
            if data[i] == 0xAA and data[i + 1] == 0x55:
                header_idx = i
                break

        if header_idx < 0:
            return None

        # 检查剩余数据长度
        remaining = len(data) - header_idx
        if remaining < 10:
            return None

        offset = header_idx + 2  # 跳过包头

        # 解析包类型 CT
        ct = data[offset]
        is_start_packet = (ct & 0x01) == 1
        if is_start_packet:
            self.scan_frequency = (ct >> 1) / 10.0
            rospy.logdebug("扫描频率: %.1f Hz" % self.scan_frequency)
        offset += 1

        # 采样点数 LSN
        lsn = data[offset]
        offset += 1

        if lsn == 0:
            return None

        # 检查数据完整性
        expected_len = header_idx + 10 + lsn * 2
        if len(data) < expected_len:
            return None

        # 起始角 FSA (小端)
        fsa = data[offset] | (data[offset + 1] << 8)
        offset += 2

        # 结束角 LSA (小端)
        lsa = data[offset] | (data[offset + 1] << 8)
        offset += 2

        # 校验码 CS (跳过，可选验证)
        cs = data[offset] | (data[offset + 1] << 8)
        offset += 2

        # 解析角度
        angle_fsa = (fsa >> 1) / 64.0  # 度
        angle_lsa = (lsa >> 1) / 64.0  # 度

        # 处理角度跨越 0 度的情况
        if angle_lsa < angle_fsa:
            angle_lsa += 360.0

        # 计算角度步进
        if lsn > 1:
            angle_step = (angle_lsa - angle_fsa) / (lsn - 1)
        else:
            angle_step = 0

        # 解析采样数据
        points = []
        for i in range(lsn):
            if offset + 2 > len(data):
                break

            # 距离数据 Si (小端)
            si = data[offset] | (data[offset + 1] << 8)
            offset += 2

            # 距离 (毫米)
            distance_mm = si / 4.0

            # 角度 (度)
            if lsn > 1:
                angle_deg = angle_fsa + angle_step * i
            else:
                angle_deg = angle_fsa

            # 角度修正
            angle_deg += self.angle_correct(distance_mm)

            # 归一化角度到 [0, 360)
            angle_deg = angle_deg % 360.0

            # 转换为弧度和米
            angle_rad = math.radians(angle_deg)
            distance_m = distance_mm / 1000.0

            # 过滤无效数据
            if distance_m >= self.range_min and distance_m <= self.range_max:
                points.append((angle_rad, distance_m))
            else:
                points.append((angle_rad, float('inf')))  # 无效点

        return points, is_start_packet, expected_len

    def publish_scan(self):
        """
        发布完整的 LaserScan 消息
        """
        if len(self.scan_points) == 0:
            return

        # 按角度排序
        self.scan_points.sort(key=lambda p: p[0])

        # 创建 LaserScan 消息
        scan_msg = LaserScan()
        scan_msg.header.stamp = rospy.Time.now()
        scan_msg.header.frame_id = self.frame_id

        # 角度范围
        scan_msg.angle_min = 0.0
        scan_msg.angle_max = 2 * math.pi

        # 计算角度增量（假设均匀分布）
        num_points = len(self.scan_points)
        if num_points > 1:
            scan_msg.angle_increment = 2 * math.pi / num_points
        else:
            scan_msg.angle_increment = 0.01

        # 时间参数
        scan_msg.time_increment = 1.0 / (self.scan_frequency * num_points) if num_points > 0 else 0
        scan_msg.scan_time = 1.0 / self.scan_frequency

        # 距离范围
        scan_msg.range_min = self.range_min
        scan_msg.range_max = self.range_max

        # 创建固定分辨率的 ranges 数组
        num_bins = 360  # 1度分辨率
        ranges = [float('inf')] * num_bins
        scan_msg.angle_increment = 2 * math.pi / num_bins

        # 将点填入对应的角度 bin
        for angle_rad, distance_m in self.scan_points:
            bin_idx = int(angle_rad / (2 * math.pi) * num_bins) % num_bins
            # 如果同一个 bin 有多个点，取最近的
            if distance_m < ranges[bin_idx]:
                ranges[bin_idx] = distance_m

        scan_msg.ranges = ranges
        scan_msg.intensities = []  # X2 不支持强度数据

        self.scan_pub.publish(scan_msg)
        rospy.logdebug("发布扫描数据: %d 点" % num_points)

    def process_data(self, data):
        """
        处理接收到的数据
        """
        self.buffer.extend(data)

        while len(self.buffer) >= 10:
            result = self.parse_packet(self.buffer)

            if result is None:
                # 没有找到有效包，尝试丢弃开头的无效数据
                # 查找下一个可能的包头
                found = False
                for i in range(1, len(self.buffer) - 1):
                    if self.buffer[i] == 0xAA and self.buffer[i + 1] == 0x55:
                        self.buffer = self.buffer[i:]
                        found = True
                        break
                if not found:
                    self.buffer = self.buffer[-1:]  # 保留最后一个字节
                break

            points, is_start, consumed = result

            # 如果是新的扫描起始包，发布上一圈数据
            if is_start and len(self.scan_points) > 0:
                self.publish_scan()
                self.scan_points = []

            # 添加点到当前扫描
            self.scan_points.extend(points)

            # 移除已处理的数据
            self.buffer = self.buffer[consumed:]

    def run(self):
        """
        主循环
        """
        rospy.loginfo("开始接收 LiDAR 数据...")

        while not rospy.is_shutdown():
            try:
                data, addr = self.sock.recvfrom(2048)  # LiDAR 数据包可能较大
                self.process_data(bytearray(data))
            except socket.timeout:
                continue
            except Exception as e:
                if not rospy.is_shutdown():
                    rospy.logerr("接收数据时出错: %s" % str(e))

        self.sock.close()
        rospy.loginfo("YDLidar UDP 接收节点已关闭")


def main():
    try:
        receiver = YDLidarReceiver()
        receiver.run()
    except rospy.ROSInterruptException:
        pass


if __name__ == '__main__':
    main()
