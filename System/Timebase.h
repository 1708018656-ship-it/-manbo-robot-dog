#ifndef __TIMEBASE_H
#define __TIMEBASE_H

#include "stm32f10x.h"

/**
 * @brief 初始化 1ms 单调时基。
 * @note 使用 TIM4，不占用 Delay.c 当前使用的 SysTick。
 */
void Timebase_Init(void);

/**
 * @brief 获取系统启动后的毫秒计数。
 */
uint32_t Timebase_NowMs(void);

/**
 * @brief 判断周期任务是否到期，并推进下一次截止时间。
 * @param now_ms 当前时间，由 Timebase_NowMs() 提供。
 * @param next_deadline_ms 任务下一次截止时间。
 * @param period_ms 任务周期，不能为 0。
 * @return 1 表示本次到期，0 表示尚未到期。
 */
uint8_t Timebase_PeriodicDue(uint32_t now_ms,
                             uint32_t *next_deadline_ms,
                             uint32_t period_ms);

#endif
