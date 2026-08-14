#ifndef __APP_TASK_H
#define __APP_TASK_H

#include "stm32f10x.h"

/**
 * @brief 初始化协作式任务调度骨架。
 */
void AppTask_Init(void);

/**
 * @brief 执行当前到期的任务。
 * @note 本函数不等待、不使用 Delay_ms/Delay_s。
 */
void AppTask_Run(uint32_t now_ms);

#endif
