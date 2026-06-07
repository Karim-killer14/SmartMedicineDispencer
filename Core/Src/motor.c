#include "motor.h"
#include "tim.h"
#include "gpio.h"

void Motor_Init(void)
{
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
}

void Motor_Forward(uint16_t speed)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);

    __HAL_TIM_SET_COMPARE(
        &htim3,
        TIM_CHANNEL_4,
        speed
    );
}

void Motor_Stop(void)
{
    __HAL_TIM_SET_COMPARE(
        &htim3,
        TIM_CHANNEL_4,
        0
    );
}
