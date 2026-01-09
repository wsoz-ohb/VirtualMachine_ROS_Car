# YDLIDAR X2

# 使用手册

![](images/8f5e761859246f96eae09211fc4fb6693b90bc9277aeff8bc06c26fdef7c0dbd.jpg)

# 目录

# 1 YDLIDAR X2 开发套件

1.1 开发套件

# 2WINDOWS下的使用操作 2

2.1 设备连接 2  
2.2 驱动安装 3  
2.3 使用评估软件

2.3.1 开始扫描 6  
2.3.2 数据保存 6  
2.3.3 显示均值和标准差 6  
2.3.4 播放和录制  
2.3.5 调试 8  
2.3.6 滤波 8

# 3 LINUX下基于ROS的使用操作 9

3.1 设备连接 9  
3.2 编译并安装YDLidar-SDK 9  
3.3 ROS驱动包安装 9  
3.4运行ydlidar_ros_driver 10  
3.5 RVIZ查看扫描结果 10  
3.6 修改扫描角度问题 11

# 4 使用注意 12

4.1 环境温度 12  
4.2 环境光照 12  
4.3 供电需求 12

# 5 修订 13

# 1 YDLIDAR X2 开发套件

YDLIDAR X2（以下简称：X2）的开发套件是为了方便用户对X2进行性能评估和早期快速开发所提供的配套工具。通过X2的开发套件，并配合配套的评估软件，便可以在PC上观测到X2对所在环境扫描的点云数据或在SDK上进行开发。

# 1.1 开发套件

X2的开发套件有如下组件：

![](images/b4a49b8217730e168b49c44de8e379cede8aa953dcf48e44d3b8c824c993c134.jpg)  
X2 激光雷达

![](images/20a222977242feac7eca548e0363414fcec35e264226df46089658aca4585dc3.jpg)  
USB Type-C数据线

![](images/c39110a703196c95dc252d3d938a9ba1bf87c7913d4d28ff542938daddcc8894.jpg)  
USB转接板  
图1YDLIDARX2开发套件

表 1 YDLIDAR X2 开发套件说明  

<table><tr><td>组件</td><td>数量</td><td>描述</td></tr><tr><td>X2 激光雷达</td><td>1</td><td>标准版本的 X2 雷达,内部集成电机驱动,可实现对电机的停转控制和电机控制。</td></tr><tr><td>USB Type-C数据线</td><td>1</td><td>配合 USB 转接板使用,连接 X2 和 PC既是供电线,也是数据线</td></tr><tr><td>USB 转接板</td><td>1</td><td>该组件实现 USB 转 UART 功能,方便 X2、PC 快速互联同时,支持串口 DTR 信号对 X2 的电机转停控制另外提供用于辅助供电的 Micro USB 电源接口(PWR)</td></tr></table>

注：USB转接板有两个接口：USB_DATA、USB_PWR。  
USB_DATA：数据供电复用接口，绝大多数情况下，只需使用这个接口便可以满足供电和通信需求。  
USB_PWR：辅助供电接口，某些开发平台的USB接口电流驱动能力较弱，这时就可以使用辅助供电。

# 2 WINDOWS下的使用操作

# 2.1 设备连接

在 windows 下对 X2 进行评估和开发时，需要将 X2 和 PC 互连，其具体过程如下：

![](images/707405cb4301ad59c11b0c70e093f0444028800a61849ebe041bc6eb3a20cfd3.jpg)  
图2YDLIDARX2设备连接STEP1

![](images/51f0e714cf9bbe6d4e1b654e26f1f13ff2e373c31fa7e8dfcf2a0bc48b73dccb.jpg)  
图3YDLIDARX2设备连接STEP2

先将转接板和X2接好，再将USB线接转接板和PC的USB端口上，注意USB线的Type-C接口接USB转接板的USB_DATA，且X2上电后进入空闲模式，电机不转。

部分开发平台或PC的USB接口的驱动电流偏弱，X2需要接入  $+5\mathrm{V}$  的辅助供电，否则雷达工作会出现异常。

![](images/05cb29d805a2a366e6563299cc4a4a0134ae6182713a63a5caccd6f07942913a.jpg)  
图4YDLIDARX2辅助供电

# 2.2 驱动安装

在 windows 下对 X2 进行评估和开发时，需要安装 USB 转接板的串口驱动。本套件的 USB 转接板采用 CP2102 芯片实现串口 (UART) 至 USB 信号的转换。其驱动程序可以在我司官网下载，或者从 Silicon Labs 的官方网站中下载：

```javascript
https://ydlidar.cn/dowfile.html?id=88
```

http://cn.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers

解压驱动包后，执行CP2102的Windows驱动程序安装文件（CP210x_VCP_Windows下的exe文件）。请根据windows操作系统的版本，选择执行32位版本(x86)，或者64位版本(x64)的安装程序。

![](images/fbb8ce9678460d3c5d0455f77ae8b2e851299d60a9ba49589b1d009d029b9211.jpg)  
图5 YDLIDAR X2驱动版本选择

双击exe文件，按照提示进行安装。

![](images/7f237f134915b6e6dc212b4ef458a0ae648e4b87d679eaebde103d8f782840f9.jpg)  
图6 YDLIDAR X2驱动安装过程

安装完成后，可以右键点击【我的电脑】，选择【属性】，在打开的【系统】界面下，选择左边菜单中的【设备管理器】进入到设备管理器，展开【端口】，可看到识别到的USB适配器所对应的串口名，即驱动程序安装成功，下图为COM3。（注意要在X2和PC互连的情况下检查端口）

![](images/9c51afb873ac6b8903fba42758ccc917c3fb54397796cbfcd6bfb710c90e573c.jpg)  
图7 YDLIDAR X2驱动安装检查

# 2.3 使用评估软件

YDLIDAR 提供了 X2 实时扫描的点云数据可视化软件 LidarViewer，用户使用该软件，可以直观的观察到 X2 的扫描效果图。YDLIDAR 上提供了 X2 实时点云数据和实时扫描频率，并且可以离线保存扫描数据至外部文件供进一步分析。可视化软件下载链接：

https://www.ydlidar.cn/Public/upload/download/T00L.zip

使用YDLIDAR前，请确保X2的USB转接板串口驱动已安装成功，并将X2与PC的USB口互连。运行评估软件：LidarViewer.exe，选择对应的串口号和型号。同时，用户也可以根据个人情况，选择语言（右上角）。

![](images/47b006befce2e7aa7bd115131da35127598b07eaa5aa5954162697ca195e554e.jpg)  
图8 YDLIDAR X2 运行评估软件

确认后，客户端的页面如下：

![](images/b218052e60e6c8c11b6323b40125c72c5d5461bf13bff10b7dedb4b3f2791e77.jpg)  
图9 客户端软件界面

# 2.3.1 开始扫描

在停止状态下点击“启动/停止”。按钮雷达会自动开始扫描，并显示环境点云，左上角显示红线位置的角度&距离信息（单位：mm），再点击一下雷达会停止扫描，如下图：

![](images/b33a16c89ce4a35a9b60f8e86151be167a27c5b7af2292b82b6f2b971e1357a1.jpg)  
图10雷达扫描点云显示

# 2.3.2 数据保存

在雷达扫描时，单击主菜单中【文件】，选择【导出到Excel】，按提示保存点云数据，系统便会以Excel格式保存扫描一圈的点云信息。

![](images/7eaed85ee3204e02e3c21ca135011a899bfae02cbb9080c8c0a165012182c883.jpg)  
图11 保存数据

# 2.3.3 显示均值和标准差

单击主菜单中【工具】，选择【均值和标准差】-【显示】。

![](images/a664b48c2b3d5cb85f543fba1c779e4fc72010272f4b46d42fa56af897d4ca69.jpg)  
图12 显示均值和标准差

根据需要选择其一，移动鼠标到测试位置，右击弹出菜单，选择【锁定鼠标追踪】。

![](images/289f28a416c2f960b83151decf4dd0f3e6b6323529eac6c9d06dd3494c1a41b1.jpg)  
图13锁定鼠标追踪

# 2.3.4 播放和录制

单击主菜单中【工具】，然后选择【记录与回放】。

![](images/d2dfaf40d88c33c02cd104c2abedf88b3654e7a6cd996b51f40f3643028959a0.jpg)  
图14 记录与回放

主窗口显示 如下：

记录激光雷达数据，点击按钮开始记录，点击按钮停止录制。

在非扫描模式下，单击按钮开始播放。

播放过程如下：

![](images/3d4bdfcbb5c20b14ed80549bd29046b4d7d284954ebcd916cbe7463ffd9d9594.jpg)  
图15播放过程

# 2.3.5 调试

单击主菜单中【工具】，然后选择【启动调试】，将原始激光雷达数据输出到“viewer_log.txt”和“viewer_log_err.txt”文件。

![](images/2faacaf2a9479cc265445f72cca9c9b7de29d0dd1427bd1c32791ea79c69eb60.jpg)  
图16 启动调试

# 2.3.6 滤波

单击主菜单中【工具】，然后选择【滤波】，增加激光雷达数据过滤算法。

![](images/8900581ab0a1a5cf6a5b5ec4e9ec237a18423bf7dbbcaaae96e6ff259723f635.jpg)  
图17 滤波设置

注：LidarViewer更多功能请点击【帮助】，选择【更多信息】，了解更多使用教程。

# 3 LINUX下基于ROS的使用操作

Linux发行版本有很多，本文仅以Ubuntu18.04、Melodic版本ROS为例。

SDK驱动程序地址：

```txt
https://github.com/YDLIDAR/YDLidar-SDK
```

ROS驱动程序地址：

```txt
https://github.com/YDLIDAR/ydlidar_ros Driver
```

# 3.1 设备连接

Linux下，X2雷达和PC互连过程和Windows下操作一致，参见Window下的设备连接。

# 3.2 编译并安装YDLidar-SDK

ydlidar_ros_driver 取决于 YDLidar-SDK 库。如果您从未安装过 YDLidar-SDK 库，或者它已过期，则必须首先安装 YDLidar-SDK 库。如果您安装了最新版本的 YDLidar-SDK，请跳过此步骤，然后转到下一步。

```powershell
$ git clone https://github.com/YDLIDAR/YDLidar-SDK.git
$ cd YDLidar-SDK/build
$ cmake ..
$ make
$ sudo make install
```

# 3.3 ROS驱动包安装

1）克隆github的ydlidar_ros_driver软件包：

```txt
$ git clone https://github.com/YDLIDAR/ydlidar_ros_driver.git ydlidar.ws/src/ydlidar_ros_driver
```

2) 构建ydlidar_ros_driver软件包：

```txt
$ cd ydlidarWs $ catkin.make
```

3）软件包环境设置：

```txt
$ source ./ devel/setup.sh
```

注意：添加永久工作区环境变量。如果每次启动新的 shell 时 ROS 环境变量自动添加到您的 bash 会话中，将很方便：

```shell
$ echo "source ~/ydlidar.ws/devel/setup.bash" >> ~/.bashrc
$ source ~/.bashrc
```

4）为了确认你的包路径已经设置，回显ROS-package_PATH变量。

```txt
$ echo $ROS-package_PATH
```

您应该看到类似以下内容：/home/tony/ydlidarWs/src:/opt/ros/melodic/share

5) 创建串行端口别名[可选]

```shell
$ chmod 0777 src/ydlidarRos_driver/startup/* $ sudo sh src/ydlidarRos_driver/startup/initenv.sh
```

注意：完成之前的操作后，请再次重新插入LIDAR。

# 3.4 运行 ydlidar_ros_driver

使用启动文件运行 ydlidar_ros_driver，例子如下：

```txt
$ roslaunch ydlidar_ros_driver X2.launch
```

# 3.5 RVIZ 查看扫描结果

运行 launch 文件，打开 rviz 查看 X2 扫描结果，如下图所示：

```txt
$ roslaunch ydlidar_ros_driver lidar_view.launch
```

注：默认以 G4 雷达为例，若使用其它型号雷达，需将 lidar_view.launch 文件中的 lidar.launch 改为对应的 **.launch 文件。（如使用 X2 雷达，需改成 X2.launch）

![](images/ff05598a41490bbdfe1433229226fd54f4f99301c6a5cac26a6cfb3e48615af0.jpg)

![](images/fcf270649ed5118c22f2c5a6db4878b39d4b36881b2278f5d7bfc830e043a1ff.jpg)  
图18YDLIDARX2雷达RVIZ运行显示  
图19X2.LAUNCH文件内容

# 3.6 修改扫描角度问题

运行 launch 文件看到的扫描数据，默认显示的是 360 度一圈的数据，若要修改显示范围，则修改 launch 内的配置参数，具体操作如下：

1）切换到对应[launch file]所在的目录下，编辑文件，其内容如图所示：

$ vim X2.launch  
```xml
<launch> <node name="ydlidar_lidar_publisher" pkg="ydlidar_ros_driver" type="ydlidar_ros_driver_node" output="screen" respawn="false"> <!-- string property --> <param name="port" type="string" value="/dev/ydlidar"/> <param name="frame_id" type="string" value="laser_frame"/> <param name="ignore_array" type="string" value=""> <!-- int property --> <param name="baudrate" type="int" value="115200"/> <!-- 0:TYPE_TOF, 1:TYPE_TRIANGLE, 2:TYPE_TOF_NET --> <param name="lidar_type" type="int" value="1"/> <!-- 0:YDLIDAR_TYPE SERIAL, 1:YDLIDAR_TYPE_TCP --> <param name="device_type" type="int" value="0"/> <param name="sample_rate" type="int" value="3"/> <param name="abnormal_check_count" type="int" value="4"/> <!-- bool property --> <param name="resolution FIXED" type="bool" value="true"/> <param name="auto reconnect" type="bool" value="true"/> <param name="reversion" type="bool" value="false"/> <param name="inverted" type="bool" value="true"/> <param name="isSingleChannel" type="bool" value="true"/> <param name="intensity" type="bool" value="false"/> <param name="support_motor_dtr" type="bool" value="true"/> <param name="invalid_range_is_inf" type="bool" value="false"/> <!-- float property --> <param name="angle_min" type="double" value "-180"/> <param name="angle_max" type="double" value="180"/> <param name="range_min" type="double" value="0.1"/> <param name="range_max" type="double" value="12.0"/> <!-- frequency is invalid, External PWM control speed --> <param name="frequency" type="double" value="10.0"/> </node> <node pkg="tf" type="static_transform_publisher" name="base_link_to_laser4" args  $= 0.0$  0.0 0.2 0.0 0.0 0.0 /basefootprint /laser_frame 40"/> </launch>
```

注意：想了解更多文件内容详细信息，请参照：

https://github.com/YDLIDAR/ydlidar ros driver#configure-ydlidar ros driver-internal-parameter

2) X2 雷达坐标在 ROS 内遵循右手定则, 角度范围为  $[-180, 180]$ , “angle_min” 是开始角度, “angle_max” 是结束角度。具体范围需求根据实际使用进行修改。

![](images/2bfb4d08c23d658c949b9580312548eb5585320ce4952ac801c2dd4c72329028.jpg)  
图20YDLIDARX2坐标角度定义

# 4 使用注意

# 4.1 环境温度

当X2工作的环境温度过高或过低，会影响测距系统的精度，并可能对扫描系统的结构产生损害，降低雷达的使用寿命。请避免在高温（>50摄氏度）以及低温（<0摄氏度）的条件中使用。

# 4.2 环境光照

X2 的理想工作环境为室内，室内环境光照（包含无光照）不会对 X2 工作产生影响。但请避免使用强光源（如大功率激光器）直接照射 X2 的视觉系统。

如果需要在室外使用，请避免X2的视觉系统直接面对太阳照射，这将这可能导致视觉系统的感光芯片出现永久性损伤，从而使测距失效。

X2 标准版本在室外强烈太阳光反射条件下的测距会带来干扰，请用户注意。

# 4.3 供电需求

在开发过程中，由于各平台的USB接口或电脑的USB接口的驱动电流可能偏低，不足以驱动X2，需要通过USB转接板上的USB_PWR接口给X2接入+5V的外部供电，不建议使用手机充电宝，部分品牌电压纹波较大。

# 5 修订

<table><tr><td>日期</td><td>版本</td><td>修订内容</td></tr><tr><td>2017-12-05</td><td>1.0</td><td>初撰</td></tr><tr><td>2018-01-22</td><td>1.1</td><td>新增辅助电源接法、文件说明、配置说明、供电需求</td></tr><tr><td>2018-04-03</td><td>1.2</td><td>适配 PointCloudViewer2.0 客户端</td></tr><tr><td>2021-08-02</td><td>1.3</td><td>适配 LidarViewer 客户端，更新 SDK、ROS 教程</td></tr></table>