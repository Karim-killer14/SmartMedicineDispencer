#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

void Motor_Init(void);
void Motor_Forward(uint16_t speed);
void Motor_Stop(void);

#endif
