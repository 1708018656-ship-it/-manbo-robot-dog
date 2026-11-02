#ifndef __COMM_TASK_H
#define __COMM_TASK_H

#include "stm32f10x.h"

void CommTask_Init(void);
void CommTask_Run(uint32_t now_ms);
/* Next queued command byte (Bluetooth first, then voice); 0 if none.
 * '5' never appears here: it is delivered as an emergency stop. */
uint8_t CommTask_PopCommand(uint8_t *command);
uint8_t CommTask_ClearCommands(void);

#endif
