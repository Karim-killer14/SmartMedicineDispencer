#include "state_machine.h"

SystemState currentState = STATE_IDLE;

void StateMachine_Run(void)
{
    switch(currentState)
    {
        case STATE_IDLE:
            break;

        case STATE_ROTATE:
            break;

        case STATE_DISPENSE:
            break;

        case STATE_WAIT_TAKE:
            break;

        case STATE_TAKEN:
            break;

        case STATE_MISSED:
            break;
    }
}
