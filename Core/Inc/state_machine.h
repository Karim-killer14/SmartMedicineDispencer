#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

typedef enum
{
    STATE_IDLE,
    STATE_ROTATE,
    STATE_DISPENSE,
    STATE_WAIT_TAKE,
    STATE_TAKEN,
    STATE_MISSED

} SystemState;

void StateMachine_Run(void);

#endif
