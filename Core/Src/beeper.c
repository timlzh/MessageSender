#include "beeper.h"

void beep_setFreq(uint16_t freq, TIM_HandleTypeDef TIM)
{	
	uint32_t Period = 1000000 / freq;
	uint16_t Pulse  = Period  / 2;
	
	 HAL_TIM_PWM_Init(&TIM);// 初始化定时器的PWM模式
	
	 HAL_TIM_PWM_Start(&TIM,TIM_CHANNEL_1);// 启动定时器的PWM模式	
	 
	/* Set the Autoreload value , frequency*/
	
	/*设置频率和占空比*/
	
	__HAL_TIM_SET_AUTORELOAD (&TIM, Period - 1);
	
	__HAL_TIM_SET_COMPARE(&TIM,TIM_CHANNEL_1,Pulse);	
}

void beep_off(TIM_HandleTypeDef TIM)
{	
	HAL_TIM_PWM_Stop(&TIM,TIM_CHANNEL_1);// 停止定时器的PWM输出
}
