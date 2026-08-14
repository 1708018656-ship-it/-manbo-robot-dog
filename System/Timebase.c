#include "Timebase.h"

#define TIMEBASE_TIMER_PRESCALER  (7200U - 1U)
#define TIMEBASE_TIMER_PERIOD     (10U - 1U)

static volatile uint32_t g_timebase_ms = 0U;

void Timebase_Init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

    TIM_InternalClockConfig(TIM4);

    TIM_TimeBaseInitTypeDef timer_init;
    timer_init.TIM_ClockDivision = TIM_CKD_DIV1;
    timer_init.TIM_CounterMode = TIM_CounterMode_Up;
    timer_init.TIM_Period = TIMEBASE_TIMER_PERIOD;
    timer_init.TIM_Prescaler = TIMEBASE_TIMER_PRESCALER;
    timer_init.TIM_RepetitionCounter = 0U;
    TIM_TimeBaseInit(TIM4, &timer_init);

    g_timebase_ms = 0U;
    TIM_ClearFlag(TIM4, TIM_FLAG_Update);
    TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);

    NVIC_InitTypeDef nvic_init;
    nvic_init.NVIC_IRQChannel = TIM4_IRQn;
    nvic_init.NVIC_IRQChannelPreemptionPriority = 1U;
    nvic_init.NVIC_IRQChannelSubPriority = 0U;
    nvic_init.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init);

    TIM_Cmd(TIM4, ENABLE);
}

uint32_t Timebase_NowMs(void)
{
    return g_timebase_ms;
}

uint8_t Timebase_PeriodicDue(uint32_t now_ms,
                             uint32_t *next_deadline_ms,
                             uint32_t period_ms)
{
    if ((next_deadline_ms == 0) || (period_ms == 0U))
    {
        return 0U;
    }

    if ((int32_t)(now_ms - *next_deadline_ms) < 0)
    {
        return 0U;
    }

    /* 以固定周期推进，避免任务因一次迟到而不断漂移。 */
    *next_deadline_ms += period_ms;
    return 1U;
}

void TIM4_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM4, TIM_IT_Update) == SET)
    {
        g_timebase_ms++;
        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
    }
}
