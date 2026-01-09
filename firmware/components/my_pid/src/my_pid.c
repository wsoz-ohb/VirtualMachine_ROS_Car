#include "my_pid.h"


//注意：PID结构体必须定义为全局变量或静态变量，然后在函数中给KP,KI,KD赋值
/************************采样周期未知且不变************************************/
//位置式PID
//pwm=Kp*e(k)+Ki*∑e(k)+Kd[e（k）-e(k-1)]
//setvalue : 设置值（期望值）
//actualvalue: 实际值
//由于全量输出，每次输出均与过去状态有关，计算时要对ek累加，计算量大
float PID_location(float setvalue, float actualvalue, PID_LocTypeDef *PID)
{ 
	PID->ek =setvalue-actualvalue;				//现误差
	PID->location_sum += PID->ek;                         //计算累计误差值
	if((PID->ki!=0)&&(PID->location_sum>(PID->out_max/PID->ki))) PID->location_sum=PID->out_max/PID->ki;
	if((PID->ki!=0)&&(PID->location_sum<(PID->out_min/PID->ki))) PID->location_sum=PID->out_min/PID->ki;//通过限制误差累计进一步积分限幅

  PID->out=PID->kp*PID->ek+(PID->ki*PID->location_sum)+PID->kd*(PID->ek-PID->ek1);		//PID计算公式
  PID->ek1 = PID->ek;				//更新前一次误差
	if(PID->out<PID->out_min)	PID->out=PID->out_min;
	if(PID->out>PID->out_max)	PID->out=PID->out_max;//PID->out限幅

	return PID->out;
}


//增量式PID
//pidout+=Kp[e（k）-e(k-1)]+Ki*e(k)+Kd[e(k)-2e(k-1)+e(k-2)]
//setvalue : 设置值（期望值）
//actualvalue: 实际值
float PID_increment(float setvalue, float actualvalue, PID_LocTypeDef *PID)
{                                
	PID->ek =setvalue-actualvalue;
  PID->out+=PID->kp*(PID->ek-PID->ek1)+PID->ki*PID->ek+PID->kd*(PID->ek-2*PID->ek1+PID->ek2);
//	PID->out+=PID->kp*PID->ek-PID->ki*PID->ek1+PID->kd*PID->ek2;
  PID->ek2 = PID->ek1;
  PID->ek1 = PID->ek;  
		
	if(PID->out<PID->out_min)	PID->out=PID->out_min;
	if(PID->out>PID->out_max)	PID->out=PID->out_max;//限幅
	
	return PID->out;
}

/*可以添加 积分限幅 积分分离  */

//PID初始化
void PID_Init(PID_LocTypeDef *PID, float kp, float ki, float kd, float out_max, float out_min)
{
    PID->kp = kp;
    PID->ki = ki;
    PID->kd = kd;
    PID->ek = 0;        // 清零当前误差
    PID->ek1 = 0;       // 清零上一次误差
    PID->ek2 = 0;       // 清零再上一次误差 (增量式会用到)
    PID->location_sum = 0; // 清零位置式积分累加项
    PID->out = 0;       // 清零输出值 (增量式会在此基础上累加)
    PID->out_max = out_max;
    PID->out_min = out_min;
}





