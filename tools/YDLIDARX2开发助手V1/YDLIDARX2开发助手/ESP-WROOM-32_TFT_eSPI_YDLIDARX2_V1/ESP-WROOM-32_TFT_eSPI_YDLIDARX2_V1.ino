/*===================================================
 * 激光雷达开发助手
 * 适用于设备模块-激光雷达YDLIDAR X2
 * Copyright (c) 2025 djq 
 * E-mail:912503010@qq.com
 * 版本:V1.0.0.1 
 * 日期:2025-01-08 10:30:10
 * ===================================================
 * 开发板ESP-WROOM-32 + TFT屏幕SPI_128X160 + YDLIDARX2激光雷达
 * 选用开发板ESP32 Wrover Module
 * 上传的时候按住上的BOOT，直到进度走完100%(有些开发板再一下BOOT键，再松开BOOT键即可)。
 * 如果上传成功，但是屏幕没反应,可以按一下开发板的EN键，再按下BOOT
 * ===================================================
 */
 /*
  #########################################################################
  ######      不要忘记配置库中的User_Setup.h文件中的对应针脚如下参数       ######
  #########################################################################
  #define ST7735_DRIVER
  #define TFT_RGB_ORDER TFT_RGB
  #define TFT_WIDTH  128
  #define TFT_HEIGHT 160
  //开发板对应针脚For ESP-WROOM-32 Dev board
  #define TFT_MOSI 23
  #define TFT_SCLK 18
  #define TFT_CS    5  // Chip select control pin
  #define TFT_DC    2  // Data Command control pin
  #define TFT_RST   4  // Reset pin (could connect to RST pin)
  #########################################################################
ST7789屏幕引脚说明
模块引脚  引脚说明
GND 液晶屏电源地
VCC 液晶屏电源正(3.3V)
SCL 液晶屏SPI总线时钟信号   SCL或SCLK或SCK或CLK
SDA 液晶屏SPI总线写数据信号 MOSI
RES 液晶屏复位控制信号（低电平复位）RESET
DC  液晶屏寄存器/数据选择控制信号（低电平：寄存器，高电平：数据）  DC或AO或RS
CS  液晶屏片选控制信号（低电平使能）
BLK 液晶屏背光控制信号（高电平点亮，如不需要控制，请接3.3V）  BLK或LED
  
  /********接线方式********
  ESP-WROOM-32  TFT屏幕SPI 128X160
  5V       >>    VCC
  GND      >>    GND
  18       >>    SCK/SCL/SCLK/CLK
  23       >>    SDA/MOSI
   2       >>    AO/RS/DC
   4       >>    RESET 
   5       >>    CS 
 *************************/

  /********接线方式********
  ESP-WROOM-32          激光雷达YDLIDAR X2
                   >>    M-CTR 电机转速控制端引脚，可不用接，接上时是电压调速或PWM调速  默认值1.8V 范围输入0V-3.3V
  GND              >>    GND
  UART2_RX/GPIO16  >>    TX   发送数据引脚
  5V               >>    VCC  注意是5V不然不够电压驱动雷达
 *************************/
 
#include <TFT_eSPI.h>              // 引入TFT_eSPI库
TFT_eSPI tft = TFT_eSPI();         // 创建TFT_eSPI对象
TFT_eSprite sprite1 = TFT_eSprite(&tft); // 创建第一个sprite对象

#include <TaskScheduler.h>         // 多任务库
#include "utility/DataCoverUils.h"  
double pi = 3.1415926535897932; //定义π常量  用PI是3.14

// 定义包头，注意修改为你的实际包头,注意这里的大小写（这里包头AA55和官方文档有出入，小心坑）
byte packetHeader[] = {0xaa,0x55};
 
// 定义缓冲区和计数器
byte buffer[1024];//定义串口的缓冲区最大值
int bufferIndex = 0; //定义串口缓冲区的计数器
int is_show_map=0;//1为开始传一圈的坐标数据，准备重新开始显示坐标

double angle_start = 135;//要检测的开始角度（用于检测障碍物）
double angle_end = 225;  //要检测的结束角度（用于检测障碍物）
double max_distance = 20;//距离障碍物的最小距离(cm):
boolean is_have_obstacles = false;//是否有障碍物
int have_obstacles_count=0;//角度范围有障碍物坐标的数量
int min_obstacles_num=3;//障碍物坐标数量大于这个值时，判断为有障碍(默认超过3个坐标点时，判断为有障碍物(最佳方案是设置为1个坐标点,但通过样本有于扰的坐标数据出现)
double scale_val=0.096;//屏幕比例系数  注：屏幕宽度128像素，刚显示最大半径为64像素，雷达原始距离值单位是mm；屏幕最大半径1m时  1m=1000mm  64/3=21.33333333333333 即圆边线间隔用20像素

void Screan_create_sprite1(){//初始第1屏
  
  //sprite1.setColorDepth(8); 
  sprite1.createSprite(128, 160);// // 创建一个128x160像素的sprite
  //sprite1.fillSprite(TFT_BLACK);//用黑色填充sprite
  //sprite1.fillScreen(TFT_BLACK);//清屏
  sprite1.drawCircle(64, 64, 60, TFT_GREEN);
  sprite1.drawCircle(64, 64, 40, TFT_GREEN);
  sprite1.drawCircle(64, 64, 20, TFT_GREEN);
  sprite1.setTextSize(1);
  sprite1.setTextColor(TFT_GREEN);
  sprite1.setTextWrap(true);
  sprite1.setCursor(44, 64 + 60);
  sprite1.print("90cm");
  sprite1.setCursor(44, 64 + 40);
  sprite1.print("60cm");
  sprite1.setCursor(44, 64 + 20);
  sprite1.print("30cm");

if(is_have_obstacles==true){//有障碍物
  sprite1.setTextColor(TFT_RED); // 设置文本颜色为白色
  sprite1.drawString("STOP", 10, 140, 1); // 在屏幕上的(60,60)位置显示文本
}else{//畅通无碍
  sprite1.setTextColor(TFT_WHITE); // 设置文本颜色为白色
  sprite1.drawString("UNBLOCK", 10, 140, 1); // 在屏幕上的(60,60)位置显示文本
}

  //绘制刻度短线
  for(int i = 0; i<360; i+= 30) {
   float sx = cos((i-90)*0.0174532925);
   float sy = sin((i-90)*0.0174532925);
   float x0 = sx*57+64;
   float yy0 = sy*57+64;
   float x1 = sx*50+64;
   float yy1 = sy*50+64;

    sprite1.drawLine(x0, yy0, x1, yy1, TFT_GREEN);
  }
  
  //绘制刻度点 60个小点及0度 90度 180度 270度大点
  for(int i = 0; i<360; i+= 6) {
   //60个小点 
   float sx = cos((i-90)*0.0174532925);
   float sy = sin((i-90)*0.0174532925);
   float x0 = sx*63+64;
   float yy0 = sy*63+64;
   sprite1.drawPixel(x0, yy0, TFT_GREEN);

    //0度 90度 180度 270度圆小实心圆(大点)
    if(i==0 || i==180) sprite1.fillCircle(x0, yy0, 1, TFT_CYAN);
    if(i==0 || i==180) sprite1.fillCircle(x0+1, yy0, 1, TFT_CYAN);
    if(i==90 || i==270) sprite1.fillCircle(x0, yy0, 1, TFT_CYAN);
    if(i==90 || i==270) sprite1.fillCircle(x0+1, yy0, 1, TFT_CYAN);
  }
}
void Screan_show_sprite1(){//显示第1屏
 
  sprite1.pushSprite(0,0);// 在屏幕上的(0,0)位置显示sprite
  sprite1.deleteSprite();// 当sprite不再需要时，可以使用deleteSprite方法删除它，删除sprite，释放内存
}

/**
 * 清除屏幕和重置文本光标
 */
void ScreanCls()
{
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(TFT_BLACK);
  tft.setTextFont(2);
}

/*
 * 任务1回调函数方法
 */
void doT1Callback()
{
 getSerialInfo(); 
}

/*
 * 任务2回调函数方法
 */
void doT2Callback()
{
 if(is_show_map==1){//重新开始一圈数据后要初始下列参数

  Screan_show_sprite1();//显示第1屏
  Screan_create_sprite1();//初始第1屏
  have_obstacles_count=0;
  is_have_obstacles=false;
  is_show_map=0;

 }
}

//任务开始////////////////////////////////////////////////////////////// 
//TASK_ONCE,执行一次
//TASK_FOREVER,永远执行

//读取串口2的通信数据
Task t1(5, TASK_FOREVER, &doT1Callback);//任务名（间隔ms，执行次数，&执行函数）

//更新初始显示屏幕
Task t2(20, TASK_FOREVER, &doT2Callback);//任务名（间隔ms，执行次数，&执行函数）

//任务调度器声明
Scheduler runner;
//任务结束////////////////////////////////////////////////////////////// 


/*
 *读取UART端口数据 
 */
void getSerialInfo(){
  if(Serial2.available()){
 //delay(50);
 while (Serial2.available() > 0) {
    int incomingByte = Serial2.read(); // 读取输入数据
   
    if (bufferIndex == 0 && incomingByte == packetHeader[0]) {// 如果缓冲区为空，且输入的字节是包头的第一个字节，则开始接收包
      buffer[bufferIndex++] = incomingByte;
    } else if (bufferIndex == 1 && incomingByte == packetHeader[1]) { // 如果缓冲区不为空且接收到的是包头的第二个字节，则接收下一个字节
      buffer[bufferIndex++] = incomingByte;
    } else if (bufferIndex > 1) { // 如果已经接收到包头，则接收包的其余部分
      buffer[bufferIndex++] = incomingByte;
      int LSN=0;//定义LSN数据长度
    if(bufferIndex>=3){ 
        LSN=(int)strtol(String(buffer[3], HEX).c_str(), NULL, 16);//读取协议里的LSN数据长度
        // 统计接收到的数据长度大于包头长度时
      if (bufferIndex >= sizeof(packetHeader) + 8+LSN*2) { //PH+CT+LSN+FSA+LSA+CS+LSN*2字节 按协议格式计算出1条完整的包数据长度      
        handlePacket(buffer, bufferIndex); // 处理完整的数据包        
        bufferIndex = 0;// 重置缓冲区
      }
     }
    } else {    
        bufferIndex = 0;// 接收的字节不是包头的一部分，重置缓冲区
    }
 }
      } 
}
 


    
    /**
     * 角度数据保存在 FSA 和 LSA 中，每一个角度数据有如下的数据结构，C 是校验位，其值固定为 1。 Ang_q2[14:7]是MSB   Ang_q2[6:0]  C是LSB     
     */
     String Angle_FSA_LSA(String HexVal)
    {
        double temp64 = 64.0f;
       // double Angle_FSA = strtol(Rshiftbit(HexVal).c_str(), NULL, 2) / temp64;  //二进制数转换为十进制数
        double Angle_FSA = Rshiftbit(HexVal) / temp64;  //换算出距离
        String resultAsString = String(Angle_FSA);
        return resultAsString;
    }

      /**  
        *  <summary>
        *  截取距离数据
        *  </summary>
        *  <param name="HexVallist">十六进制符串</param>
        *  <param name="i">截取字符的长度</param>
        *  <returns></returns>
        **/ 
         String Distance_Data_i(String HexVallist,int i)
        {   
            String result = "0000";
            result = HexVallist.substring((i-1)*4,(i-1)*4+4);
            return result;
        }


/**
 *圆半径转换为角度
 */
double radiusToDegrees(double radius) {
  double degrees = (radius / pi) * 180.0; 
  return degrees;
}




//数据报文样本///////////////////////////////////////////////////////////////////
//AA55010101000100AB540000 上电后,第1条报文                                  CT_BinaryString_Val的值就是[00000001]  CT_BinaryString_Val.Substring(7, 1) 即 CT[bit(0)]=1
//AA556F0103AC03ACC5540000 起始数据包的数据报文                               CT_BinaryString_Val的值就是[01101111]  CT_BinaryString_Val.Substring(7, 1) 即 CT[bit(0)]=1 //起始数据包(雷达开始传输扫描的坐标数据)
//AA55060821AC21ACA05824061C061C061C06000030052C052805 角度数据包的数据报文   CT_BinaryString_Val的值就是[00000110]  CT_BinaryString_Val.Substring(7, 1) 即 CT[bit(0)]=0 //点云数据包.就是坐标数据
////////////////////////////////////////////////////////////////////////////////

/**
 * 数据包的类型
 */
int CTDataType(String list_data){
  int data_type=0; 
   String CT = list_data.substring(4, 6);//包类型:表示当前数据包的类型，CT[bit(0)]=1 表示为一圈数据起始，CT[bit(0)]=0 表示为点云数据包，CT[bit(7:1)]为预留位
   //String CT_BinaryString_Val = Convert.ToString(Convert.ToInt32(CT, 16), 2).PadLeft(8, '0');//把16进制转换为二进制8bit的字符串，二进制不足8位时，左边补零
  String CT_BinaryString_Val = hexToBitXBin(CT,8); //把16进制转换为二进制8bit的字符串，二进制不足8位时，左边补零
   //Serial.println("CT_BinaryString_Val:"+CT_BinaryString_Val+" ");
   String data_type_str = CT_BinaryString_Val.substring(7, 8);//这里的Substring(6, 8) 就是CT[bit(0)]=0 时，表明该包数据为点云数据包; CT[bit(0)]=1 时，表明该包数据为起始数据包，例如：AA556F0103AC03ACC5540000 [01101111]  这个是收到起始数据包的数据  
   //Serial.println("CT_Data_Type:"+CT_Data_Type+" ");
  if(data_type_str == "0")//点云数据包
  {
  data_type=0;
  }else if (data_type_str == "1")//起始数据包
  {
   data_type=1;
  }
                                        

return data_type;
}

        /** <summary>
        /// 校验码（计算异或校验码）
        /// </summary>
        /// <param name="list_data">十六进制符串报文</param>
        /// <returns></returns>
        **/
         boolean CheckSum(String list_data)
        {
            boolean result = false;
 
            String PH = list_data.substring(0, 4);//数据包头:长度为 2B，固定为 0x55AA，低位在前，高位在后
            String CT = list_data.substring(4, 6);//包类型:表示当前数据包的类型，CT[bit(0)]=1 表示为一圈数据起始，CT[bit(0)]=0 表示为点云数据包，CT[bit(7:1)]为预留位
            String LSN = list_data.substring(6, 8);//采样数量:当前数据包中包含的采样点数量；起始数据包中只有 1 个起始点的数据，该值为1; 16进制4位字符即2字节
            String FSA = list_data.substring(8, 12);//起始角:采样数据中第一个采样点对应的角度数据
            String LSA = list_data.substring(12, 16);//结束角:采样数据中最后一个采样点对应的角度数据
            String Check_Sum = list_data.substring(16, 20);//校验码
            //Serial.println("Check_Sum:"+Check_Sum+" ");
            int LSN_Length = (int)strtol(LSN.c_str(), NULL, 16);//距离数据有多少个，4个十六进制字符即2字节为1个距离数据长度(这里做十六进制转十进制)
            String Si = list_data.substring(20, LSN_Length * 4+20); //采样数据

            int PH_i  = (int)strtol(PH.c_str(), NULL, 16);
            int FSA_i = (int)strtol(FSA.c_str(), NULL, 16);
            int CT_LSN_i = (int)strtol(String(CT + LSN).c_str(), NULL, 16);
            int LSA_i = (int)strtol(LSA.c_str(), NULL, 16);
            int checksum = PH_i ^ FSA_i;//计算异或
            for (int i = 1; i <= LSN_Length; i ++)
            {
             checksum = checksum ^  (int)strtol(Si.substring((i-1)*4,(i-1)*4+4).c_str(), NULL, 16);//计算异或
            }
             checksum = checksum ^ CT_LSN_i ^ LSA_i;//计算异或校验码
            //Serial.println("checksum:"+String(checksum)+"");
            String hexChecksum = intTohex(checksum);//10进制转16进制
            //Serial.println("CheckSum:"+Check_Sum+" hexChecksum:"+hexChecksum+" ");
            if (Check_Sum == hexChecksum)
            {
                result = true;
            }
  
            return result;
        } 

       /**
        * <summary>
        * 角度范围内是否有障碍物
        * </summary>
        * <param name="angle_start"></param>
        * <param name="angle_end"></param>
        * <param name="max_distance"></param>
        * <param name="current_angle"></param>
        * <param name="current_distance"></param>
        * <returns></returns>
        **/ 
        boolean is_allowable_range(double angle_start, double angle_end, double max_distance, double current_angle, double current_distance)
        {
           boolean is_in_range = false;
            if (current_angle > angle_start && current_angle < angle_end)
            {
                if (current_distance <= max_distance)
                {
                    is_in_range = true;
                }
            }
            return is_in_range;
        }
    
/**<summary>
*  按数据协议解析报文数据
* </summary>
* <param name="list_data"></param>
**/ 
void DataInfoDistanceAngle(String list_data) {

            String PH = list_data.substring(0, 4);//数据包头:长度为 2B，固定为 0x55AA，低位在前，高位在后
            String CT = list_data.substring(4, 6);//包类型:表示当前数据包的类型，CT[bit(0)]=1 表示为一圈数据起始，CT[bit(0)]=0 表示为点云数据包，CT[bit(7:1)]为预留位
            String LSN = list_data.substring(6, 8);//采样数量:当前数据包中包含的采样点数量；起始数据包中只有 1 个起始点的数据，该值为1; 16进制4位字符即2字节           
            
            String FSA = list_data.substring(8, 12);//起始角:采样数据中第一个采样点对应的角度数据
            FSA = FSA.substring(2, 4) + FSA.substring(0, 2);//采样数据，高位和低位要处理互换位置（重要）
            
            String LSA = list_data.substring(12, 16);//结束角:采样数据中最后一个采样点对应的角度数据
            LSA = LSA.substring(2, 4) + LSA.substring(0, 2);//采样数据，高位和低位要处理互换位置（重要）
         
            String Check_Sum = list_data.substring(16, 20);//校验码

            int LSN_Length = (int)strtol(LSN.c_str(), NULL, 16);//距离数据有多少个，4个十六进制字符即2字节为1个距离数据长度(这里做十六进制转十进制)
            String Si = list_data.substring(20, LSN_Length * 4+20); //采样数据
            double D_X = 4.0f;
            String Angle_FSA_Temp = Angle_FSA_LSA(FSA);//起始角度值(一级解析)

            String Angle_LSA_Temp = Angle_FSA_LSA(LSA);//结束角度值(一级解析)

             //////////////////开始(二级解析)//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 
            double Angle_Diff_val = Angle_LSA_Temp.toDouble() - Angle_FSA_Temp.toDouble();//顺时针角度差值
            if (Angle_LSA_Temp.toDouble() < Angle_FSA_Temp.toDouble())//当起始角度大于结束角度时，例如：从305度至45度时,有6个间隔 即这里的i=6   (Angle_Diff_val / (6 - 1)) * (i - 1）
            {
                Angle_Diff_val = 360 - Angle_FSA_Temp.toDouble() + Angle_LSA_Temp.toDouble();//起始角度~结束角度，从高角度到低角度时，换算角度差值的方法
            }else if (Angle_LSA_Temp.toDouble() == Angle_FSA_Temp.toDouble()){
              Angle_Diff_val=0;
            }else{ 
                    
            }

          // Serial.println("采样16进制:    起始角度：" + Angle_FSA_Temp + " 结束角度值：" + Angle_LSA_Temp + " 角度差值：" + String(Angle_Diff_val) + " 距离个：" + String(LSN_Length) + "  距离数据：" + String(Si) + "  ");

            for (int i = 1; i <= LSN_Length; i++)//1,2,3,4,5......LSN 
            { 
                String Si_item = Distance_Data_i(Si, i);
                String item_LSB_MSB = Si_item.substring(2, 4) + Si_item.substring(0, 2);// Si 为采样数据。设采样数据为 E5 6F，由于本系统是小端模式，所以本采样点 S = 0x6FE5(这里LSB与MSB高低位置要互换)，带入到距离解算公式，得 Distance = 7161.25mm。              
                double Distance_i = (int)strtol(item_LSB_MSB.c_str(), NULL, 16) / D_X;//距离解算公式  这里的原始值的单位是mm
               if (Distance_i>0)
              {   
                double angle_i = ((Angle_Diff_val / (LSN_Length - 1)) * (i - 1)) + Angle_FSA_Temp.toDouble();//中间角度值 即起始角度 至 结束角度 之间按距离数据的个数，平分的角度
                if (angle_i > 360)
                {
                    angle_i = angle_i - 360;//起始角度~结束角度，从高角度到低角度时，换算角度的方法
                }

                double AngCorrect_i = 0;//角度修正值
 
                if (Distance_i == 0)
                {
                    AngCorrect_i = 0;
                }
                else
                {
                    double Dis_Val = 21.8 * ((155.3 - Distance_i) / (155.3 * Distance_i));//本设备需要计算的距离值                  
                    AngCorrect_i = radiusToDegrees(Dis_Val);  // 将圆半径 转换成角度
                }


                double OK_Angle_i = 0;
                 OK_Angle_i = angle_i + AngCorrect_i;
                 //////////////////结束(二级解析)//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

                 double dw_Angle_i=OK_Angle_i-90;//这里要反时针旋转90度
                 if (Distance_i>0)
                 {
                  
                double dpx = (Distance_i*scale_val * cos((dw_Angle_i* pi) / 180.0 ));//距离*比例系数
                double dpy = (Distance_i*scale_val * sin((dw_Angle_i* pi) / 180.0));//距离*比例系数
                //Serial.println("" + String(i) + " 采样16进制:" + String(Si_item) + " 角度：" + String(OK_Angle_i) + " 距离：" + String(Distance_i) + "mm 中间角度值：" + String(angle_i) + "  修正角度值：" + String(AngCorrect_i) + " dpx:"+dpx+" dpy:"+dpy+" ");
               
            
                           boolean is_in_range = is_allowable_range(angle_start, angle_end, max_distance * 10   , OK_Angle_i , Distance_i);//这里要顺时针旋转90度  max_distance的单位是mm所以max_distance * 10是转成mm Distance_i的单位是mm

                            if (is_in_range == true)
                            {
                                 have_obstacles_count++;
                                //tft.drawPixel(64+dpx,64+dpy, TFT_RED);
                                // tft.fillCircle(64+dpx,64+dpy, 1, TFT_RED);
//                                if(sprite_x=1){
//                                 sprite1.fillCircle(64+dpx,64+dpy, 1, TFT_RED);
//                                }if(sprite_x=2){
//                                 sprite2.fillCircle(64+dpx,64+dpy, 1, TFT_RED);
//                                }
                               sprite1.fillCircle(64+dpx,64+dpy, 1, TFT_RED);
                                 //Serial.println("" + String(i) + " "+angle_start+"~"+angle_end+" 画图角度:" + String(dw_Angle_i) + " 原角度：" + String(OK_Angle_i) + " 原距离：" + String(Distance_i) + "mm  比例距离：" + String(Distance_i*scale_val) + "m   最小距离：" + String(max_distance * 10) + "mm  中间角度值：" + String(angle_i) + "  修正角度值：" + String(AngCorrect_i) + " dpx:"+dpx+" dpy:"+dpy+" ");
                
                            }else{
                                //tft.fillCircle(64+dpx,64+dpy, 1, TFT_YELLOW);
//                                 //tft.drawPixel(64+dpx,64+dpy, TFT_YELLOW);
//                                  if(sprite_x=1){
//                                 sprite1.fillCircle(64+dpx,64+dpy, 1, TFT_YELLOW);
//                                }if(sprite_x=2){
//                                 sprite2.fillCircle(64+dpx,64+dpy, 1, TFT_YELLOW);
//                                }
                                sprite1.fillCircle(64+dpx,64+dpy, 1, TFT_YELLOW);
                            }
                
                   
                   }

            }
                   
             }
    
             
    


             
}

 
/*
 * 初始化屏幕显示
 */
 
void ScreanInitialize() {
  tft.fillScreen(TFT_BLACK);//清屏
  tft.drawCircle(64, 64, 60, TFT_GREEN);
  tft.drawCircle(64, 64, 40, TFT_GREEN);
  tft.drawCircle(64, 64, 20, TFT_GREEN);
  tft.setTextSize(1);
  tft.setTextColor(TFT_GREEN);
  tft.setTextWrap(true);
  tft.setCursor(44, 64 + 60);
  tft.print("90cm");
  tft.setCursor(44, 64 + 40);
  tft.print("60cm");
  tft.setCursor(44, 64 + 20);
  tft.print("30cm");

if(is_have_obstacles==true){//有障碍物
  tft.setTextColor(TFT_RED); // 设置文本颜色为白色
  tft.drawString("STOP", 10, 140, 1); // 在屏幕上的(60,60)位置显示文本
}else{//畅通无阻
  tft.setTextColor(TFT_WHITE); // 设置文本颜色为白色
  tft.drawString("UNBLOCK", 10, 140, 1); // 在屏幕上的(60,60)位置显示文本
}

  //绘制刻度短线
  for(int i = 0; i<360; i+= 30) {
   float sx = cos((i-90)*0.0174532925);
   float sy = sin((i-90)*0.0174532925);
   float x0 = sx*57+64;
   float yy0 = sy*57+64;
   float x1 = sx*50+64;
   float yy1 = sy*50+64;

    tft.drawLine(x0, yy0, x1, yy1, TFT_GREEN);
  }
  
  //绘制刻度点 60个小点及0度 90度 180度 270度大点
  for(int i = 0; i<360; i+= 6) {
   //60个小点 
   float sx = cos((i-90)*0.0174532925);
   float sy = sin((i-90)*0.0174532925);
   float x0 = sx*63+64;
   float yy0 = sy*63+64;
   tft.drawPixel(x0, yy0, TFT_GREEN);

    //0度 90度 180度 270度圆小实心圆(大点)
    if(i==0 || i==180) tft.fillCircle(x0, yy0, 1, TFT_CYAN);
    if(i==0 || i==180) tft.fillCircle(x0+1, yy0, 1, TFT_CYAN);
    if(i==90 || i==270) tft.fillCircle(x0, yy0, 1, TFT_CYAN);
    if(i==90 || i==270) tft.fillCircle(x0+1, yy0, 1, TFT_CYAN);
  }
  
}



/**
 * 处理完整的数据包
 */
void handlePacket(byte packet[], int length) {
  // 解析数据包，这里只是简单地打印出数据
  String hexlist ="";
  for (int i = 0; i < length; i++) {
    String hex_val=String(packet[i], HEX);
    if (hex_val.length() < 2) { 
      hex_val = '0' + hex_val; //不足两位补零 
    }
     hexlist=hexlist+hex_val; 
  }
   hexlist.toUpperCase();//字符转为大写
   //Serial.println(hexlist);
   boolean ishave = CheckSum(hexlist);
   if(ishave==true){
     int CT_Data_Type =  CTDataType(hexlist);
    if(CT_Data_Type==0){
           DataInfoDistanceAngle(hexlist);//处理1条完整包的报文数据
    }else if(CT_Data_Type==1){
       if(have_obstacles_count>min_obstacles_num){//默认超过3个坐标点时，判断为有障碍物
           is_have_obstacles=true;
        }
      
     is_show_map = 1;

    }


    
   }
    
 
}

void setup()
{
  Serial.begin(115200);
  //将RPLIDAR驱动与ESP32 的 Serial2 硬件串口绑定。
  //Serial2.begin(115200, SERIAL_8N1, 16, 17);//需要改引脚就要用到这个
  Serial2.begin(115200);//串口通信中，TX（发送）和RX（接收）引脚需要交叉连接, 注意设备 YDLIDAR X2 的引脚 TX（发送） RX（接收）

  Serial.println("FOR YDLIDAR X2 ");
  Serial.println("串口通信中，TX（发送）和RX（接收）引脚需要交叉连接, 注意设备 YDLIDAR X2 的引脚 TX（发送） RX（接收）");
  Serial.println("从角度:"+String(angle_start)+"~"+String(angle_end)+" 距离障碍物的最小距离(cm):" + String(max_distance)+ "cm    屏幕比例系数：" + String(scale_val) + " 屏幕比例系数  注：屏幕宽度128像素，刚显示最大半径为64像素，雷达原始距离值单位是mm；屏幕最大半径1m时  1m=1000mm  64/3=21.33333333333333 即 圆周边线的间隔用20像素 ");
 
  //Serial.println(PI);
   tft.begin();
   // 设置屏幕旋转角度为90度
   tft.setRotation(2); //参数可以是0、1、2、3这四个值，分别代表屏幕旋转0度、90度、180度和270度
   ScreanCls();
   tft.printf("FOR YDLIDAR X2.\n");
   delay(1000);
 
 
   ScreanInitialize();
///////////////////
   //调度器初始化
   runner.init();
   Serial.println("初始化任务调度器");
  
   //添加任务
   runner.addTask(t1);//注册任务1
   Serial.println("added t1 读取串口通信数据任务运行");
   runner.addTask(t2);
   Serial.println("added t2 显示屏幕任务运行");

   //使用任务
   t1.enable();
   t2.enable();

///////////////////////////////

}

void loop()
{ 

runner.execute();
  
}
