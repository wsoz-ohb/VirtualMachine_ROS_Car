#ifndef MY_PID_H
#define MY_PID_H
#include <math.h>

typedef struct
{
  float kp;                       //比例系数Proportional
  float ki;                       //积分系数Integral
  float kd;                       //微分系数Derivative
//	float ti;                       //积分时间常数
//  float td;                       //微分时间常数
//	float period;										//采样周期
  float ek;                       //当前误差
  float ek1;                      //前一次误差e(k-1)
  float ek2;                      //再前一次误差e(k-2)
  float location_sum;             //累计积分位置
	float out;											//PID输出值
  float out_max;                  //PID输出最大值
  float out_min;                  //PID输出最小值
}PID_LocTypeDef;
void PID_Init(PID_LocTypeDef *PID, float kp, float ki, float kd, float out_max, float out_min);
float PID_location(float setvalue, float actualvalue, PID_LocTypeDef *PID);
float PID_increment(float setvalue, float actualvalue, PID_LocTypeDef *PID);

#endif // !PID_H
