#ifndef __SAFETY_TASK_H
#define __SAFETY_TASK_H

#include "stm32f10x.h"

void SafetyTask_Init(void);
void SafetyTask_Run(uint32_t now_ms);

void SafetyTask_RequestEmergencyStop(void);
void SafetyTask_RequestCommunicationFault(void);
uint8_t SafetyTask_ConsumeStopRequest(void);
uint8_t SafetyTask_IsFaultActive(void);

#endif
