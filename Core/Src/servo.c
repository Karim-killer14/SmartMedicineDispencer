#include "servo.h"
#include "tim.h"

void Servo_Init(void)
{
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
}

void Servo_Open(void)
{
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 2000);
}

void Servo_Close(void)
{
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 1000);
}
