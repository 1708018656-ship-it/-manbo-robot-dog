#ifndef __COMM_TASK_H
#define __COMM_TASK_H

#include "stm32f10x.h"

void CommTask_Init(void);
void CommTask_Run(uint32_t now_ms);

#endif
