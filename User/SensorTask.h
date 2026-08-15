#ifndef __SENSOR_TASK_H
#define __SENSOR_TASK_H

#include "stm32f10x.h"

void SensorTask_Init(void);
void SensorTask_Run(uint32_t now_ms);
void SensorTask_SetEnabled(uint8_t enabled);
uint8_t SensorTask_IsValid(void);
uint16_t SensorTask_LastDistanceCm(void);

#endif
