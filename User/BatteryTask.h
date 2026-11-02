#ifndef BATTERY_TASK_H
#define BATTERY_TASK_H

#include <stdint.h>
#include "Battery.h"

void BatteryTask_Init(void);
void BatteryTask_Run(uint32_t now_ms);      /* call every few ms; samples at 10 Hz */
BatteryLevel BatteryTask_Level(void);
uint16_t BatteryTask_Millivolts(void);
/* Returns the new level once each time it changes, else BATTERY_UNKNOWN. */
uint8_t BatteryTask_ConsumeChange(BatteryLevel *new_level);

#endif
