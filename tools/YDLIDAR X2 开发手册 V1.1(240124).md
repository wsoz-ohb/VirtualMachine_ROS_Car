# YDLIDAR X2

# 开发手册

![](images/a4d6d70be64c58280f0ce3f119f0a74a09d266028e4bbce7667182bd77f96e64.jpg)

# 目录

1 工作机制  
2 采样测距. 1  
3 上电信息 2  
4 数据协议 2  
5 速度控制. 5  
6修订 6

# 1 工作机制

X2上电后，系统自动启动测距，以下是X2系统的工作流程：

![](images/af61a323a07719acc8bad346f2957744aec989eac5f73b6465be5bed4835a866.jpg)  
图1YDLIDARX2系统工作流程

# 2 采样测距

在上电后，系统会自动启动测距，同时会向串口输出一次启动扫描的报文数据：A5 5A05 00 00 40 81。该报文具体含义如下：

![](images/49c3e562deade2f8541b483065ab432a899a7d3aecae2773111753448f727bec.jpg)  
图2YDLIDARX2启动扫描报文说明

$\succ$  起始标志：X2的报文标志统一为0xA55A；  
应答长度：应答长度表示的是应答内容的长度，但当应答模式为持续应答时，长度应为无限大，因此该值失效，启动扫描的报文应答长度为无限大；  
应答模式：该位只有 2bits，表示本次报文是单次应答或持续应答，启动扫描的应答模式为1，其取值和对应的模式如下：

表 1 X2 应答模式取值和对应应答模式  

<table><tr><td>应答模式取值</td><td>0x0</td><td>0x1</td><td>0x2</td><td>0x3</td></tr><tr><td>应答模式</td><td>单次应答</td><td>持续</td><td colspan="2">未定义</td></tr></table>

$\succ$  类型码：启动扫描报文的类型码为  $0 \times 81$  
应答内容：扫描数据，详见数据协议。

# 3 上电信息

在上电后，系统会输出一次上电信息，会反馈设备的型号、固件版本和硬件版本，以及设备出厂序列号。其应答报文为：

![](images/69603b16abbcb2e2550b5b55fe4d107d8853552becec23296c34eca013c0c961.jpg)  
图3 YDLIDARX2设备信息报文示意图

按照协议解析：应答长度  $= 0\mathrm{x}00000014$  ，应答模式  $= 0\mathrm{x}0$  ，类型码  $= 0\mathrm{x}04$  。

即应答内容字节数为 20; 本次应答为单次应答, 类型码为 04, 该类型应答内容满足一下数据结构:

![](images/dd7643253c674818c9f4185a591823c463931b328b230278ebaa9e3a72904a70.jpg)  
图4YLDIDARX2设备信息应答内容数据结构示意图

$\succ$  型号：1个字节设备机型，如X2的机型代号是04；  
> 固件版本：2个字节，低字节为主版本号，高字节为次版本号；  
> 硬件版本：1个字节，代表硬件版本；  
$\succ$  序列号：16个字节，唯一的出厂序列号。

# 4 数据协议

系统启动扫描后，会在随后的报文中输出扫描数据，其数据协议按照以下数据结构，以16进制向串口发送至外部设备。

字节偏移：

![](images/c3dd671b3cfd495aefe464f2c78aea2d1638a7c38148b7fa810247ac00640a76.jpg)  
图5扫描命令应答内容数据结构示意图

表 2 扫描命令应答内容数据结构描述  

<table><tr><td>内容</td><td>名称</td><td>描述</td></tr><tr><td>PH(2B)</td><td>数据包头</td><td>长度为2B，固定为0x55AA，低位在前，高位在后</td></tr><tr><td>CT(1B)</td><td>包类型</td><td>表示当前数据包的类型，CT[bit(0)]=1表示为一圈数据起始，CT[bit(0)]=0表示为点云数据包，CT[bit(7:1)]为预留位</td></tr><tr><td>LSN(1B)</td><td>采样数量</td><td>表示当前数据包中包含的采样点数量；起始数据包中只有1个起始点的数据，该值为1</td></tr><tr><td>FSA(2B)</td><td>起始角</td><td>采样数据中第一个采样点对应的角度数据</td></tr><tr><td>LSA(2B)</td><td>结束角</td><td>采样数据中最后一个采样点对应的角度数据</td></tr><tr><td>CS(2B)</td><td>校验码</td><td>当前数据包的校验码，采用双字节异或对当前数据包进行校验</td></tr><tr><td>Si(2B)</td><td>采样数据</td><td>系统测试的采样数据，为采样点的距离数据，其中Si节点的LSB中还集成了干扰标志</td></tr></table>

# $\succ$  起始位&扫描频率解析：

当检测到CT[bit(0)]=0时，表明该包数据为点云数据包；

当检测到  $\mathrm{CT}[\mathrm{bit}(0)] = 1$  时，表明该包数据为起始数据包，该数据包中  $\mathrm{LSN} = 1$  ，即Si的数量为1；其距离、角度的具体值解析参见下文；同时，起始数据包中，  $\mathrm{CT}[\mathrm{bit}(7:1)]$  扫描频率信息，  $\mathrm{F} = \mathrm{CT}[\mathrm{bit}(7:1)] / 10$  （当  $\mathrm{CT}[\mathrm{bit}(7:1)] = 1$  时）。

注：当CT[bit(7:1)] = 0时，CT[bit(7:1)]为预留位，未来版本会用作其他用途，因此在解析CT过程中，只需要对bit(0)位做起始帧的判断。

# 距离解析：

距离解算公式：  $\mathrm{Distance}_i = \frac{Si}{4}$

其中，Si为采样数据。设采样数据为E56F，由于本系统是小端模式，所以本采样点S $= 0\mathrm{x}6\mathrm{FE}5$  ，带入到距离解算公式，得Distance  $= 7161.25\mathrm{mm}$  。

# $\succ$  角度解析：

角度数据保存在 FSA 和 LSA 中，每一个角度数据有如下的数据结构，C 是校验位，其值固定为 1。角度解析有两个等级：一级解析和二级解析。一级解析初步得到角度初值，二级解析对角度初值进行修正，具体过程如下：

一级解析：

起始角解算公式：  $Angle_{FSA} = \frac{Rshiftbit(FSA,1)}{64}$

![](images/25d6f9b6b43cafa52f78b91fc071bd8fd6c6be9280a4324428792c9f9a328796.jpg)  
图6角度数据结构示意图

结束角解算公式：  $Angle_{LSA} = \frac{Rshiftbit(LSA,1)}{64}$

中间角解算公式：  $Angle_{i} = \frac{diff(Angle)}{LSN - 1} * (i - 1) + Angle_{FSA} \qquad (i = 2,3,\dots,LSN - 1)$

Rshiftbit(data,1)表示将数据 data 右移一位。diff(Angle) 表示起始角（未修正值）到结束角（未修正值）的顺时针角度差，LSN 表示本帧数据包采样数量。

# 二级解析：

角度修正公式：  $Angle_{i} = Angle_{i} + \mathrm{AngCorrect}_{i}$  （ $i = 1,2,\ldots,LSN$ ）

其中，AngCorrect为角度修正值，其计算公式如下， $\mathrm{tand}^{-1}$ 为反三角函数，返回角度值：

IF  $\mathrm{Distance}_i = = 0$  AngCorrect  $i = 0$

ELSE  $\mathrm{AngCorrect}_i = \mathrm{tand}^{-1}(21.8*\frac{155.3 - \mathrm{Distance}_i}{155.3*\mathrm{Distance}_i})$

设数据包中，第  $4\sim 8$  字节为28E56FBD79，所以  $\mathrm{LSN} = 0\mathrm{x}28 = 40(\mathrm{dec})$  ，  $\mathrm{FSA} = 0\mathrm{x}6\mathrm{FE}5$  ，  $\mathrm{LSA} = 0\mathrm{x}79\mathrm{BD}$  ，带入一级解算公式，得：

$$
\begin{array}{l} A n g l e _ {F S A} = 2 2 3. 7 8 ^ {\circ}, A n g l e _ {L S A} = 2 4 3. 4 7 ^ {\circ}, d i f f (A n g l e) = 1 9. 6 9 ^ {\circ} \\ A n g l e _ {i} = \frac {1 9 . 6 9 ^ {\circ}}{3 9} * (i - 1) + 2 2 3. 7 8 ^ {\circ} \quad (i = 2, 3, \dots , 3 9) \\ \end{array}
$$

假设该帧数据中， $\mathrm{Distance}_1 = 1000$ ， $\mathrm{Distance}_{LSN} = 8000$ ，带入二级解算公式，得：

$\mathrm{AngCorrect}_1 = -6.7622^\circ$ ， $\mathrm{AngCorrect}_{LSN} = -7.8374^\circ$ ，所以：

$$
A n g l e _ {F S A} = A n g l e _ {1} + \text {A n g C o r r e c t} _ {1} = 2 1 7. 0 1 7 8 ^ {\circ}
$$

$$
A n g l e _ {L S A} = A n g l e _ {L S A} + \text {A n g C o r r e c t} _ {L S A} = 2 3 5. 6 3 2 6 ^ {\circ}
$$

同理， $A n g l e_{i}$ $(i = 2,3,\dots,LSN - 1)$ ，可以依次求出。

# 校验码解析：

校验码采用双字节异或，对当前数据包进行校验，其本身不参与异或运算，且异或顺序不是严格按照字节顺序，其异或顺序如图所示，因此，校验码解算公式为：

$$
\mathrm {C S} = \mathrm {X O R} _ {1} ^ {e n d} (C _ {i}) \qquad \qquad i = 1, 2, \ldots , e n d
$$

![](images/2e8ba0da439324df33fb65dacfe70fdad467d15713a22733bb82e99c45311adc.jpg)  
图7CS异或顺序示意图

$\mathrm{XOR}_{1}^{end}$  为异或公式，表示将元素中从下标 1 到 end 的数进行异或。但异或满足交换律，实际解算中可以无需按照本文异或顺序。

# 5 速度控制

同时，用户可以根据实际需要，改变扫描频率来满足需求。通过改变M_SCTP管脚输入电压，或改变输入的PWM信号的占空比，来调控电机转速（具体控制方法，请参考数据手册）。

# 6 修订

<table><tr><td>日期</td><td>版本</td><td>修订内容</td></tr><tr><td>2019-04-24</td><td>1.0</td><td>初撰</td></tr><tr><td>2021-07-30</td><td>1.1</td><td>优化CT信息</td></tr></table>