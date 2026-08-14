#ifndef __ACTION_TASK_H
#define __ACTION_TASK_H

#include "stm32f10x.h"

typedef enum
{
    ACTION_NONE = 0,
    ACTION_STAND,
    ACTION_FORWARD,
    ACTION_BACKWARD,
    ACTION_TURN_LEFT,
    ACTION_TURN_RIGHT,
    ACTION_SWING_FORWARD_BACK,
    ACTION_SWING_LEFT_RIGHT,
    ACTION_DANCE,
    ACTION_HELLO,
    ACTION_STRETCH,
    ACTION_TWO_HANDS,
    ACTION_LAZY,
    ACTION_HEAD_UP,
    ACTION_SLEEP_PRONE,
    ACTION_SLEEP_SIDE,
    ACTION_DECLARE_LOVE,
    ACTION_ELEMENTS,
    ACTION_TRAINING,
    ACTION_WORLD_LIGHT,
    ACTION_WAKE_UP
} ActionId;

void ActionTask_Init(void);
void ActionTask_Run(uint32_t now_ms);
uint8_t ActionTask_RequestMotion(ActionId action);
uint8_t ActionTask_HandleCommand(uint8_t command);
void ActionTask_AbortToSafeStop(void);
uint8_t ActionTask_IsActive(void);
ActionId ActionTask_Current(void);

#endif
