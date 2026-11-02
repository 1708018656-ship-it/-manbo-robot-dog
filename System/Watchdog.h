#ifndef WATCHDOG_H
#define WATCHDOG_H

/*
 * Independent watchdog (IWDG), clocked by the ~40 kHz LSI.
 * Prescaler 64 -> 625 Hz, reload 1000 -> ~1.6 s timeout
 * (LSI tolerance 30..60 kHz gives 1.07 s .. 2.1 s).
 *
 * Safe to enable only because the main loop no longer contains multi-second
 * Delay_s() calls. Call Watchdog_Init() after all blocking start-up code,
 * then Watchdog_Feed() once per main-loop iteration.
 *
 * The counter is frozen while the core is halted by a debugger, so
 * breakpoints do not trigger resets.
 *
 * Define MANBO_NO_WATCHDOG to disable (e.g. while bringing up new hardware).
 */

#include "stm32f10x.h"

#define WATCHDOG_RELOAD 1000U

static inline void Watchdog_Init(void)
{
#ifndef MANBO_NO_WATCHDOG
    DBGMCU->CR |= DBGMCU_CR_DBG_IWDG_STOP;
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
    IWDG_SetPrescaler(IWDG_Prescaler_64);
    IWDG_SetReload(WATCHDOG_RELOAD);
    IWDG_ReloadCounter();
    IWDG_Enable();
#endif
}

static inline void Watchdog_Feed(void)
{
#ifndef MANBO_NO_WATCHDOG
    IWDG_ReloadCounter();
#endif
}

/* Non-zero if the last reset was caused by the watchdog; clears the flags. */
static inline uint8_t Watchdog_CausedLastReset(void)
{
    uint8_t by_iwdg = (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET) ? 1U : 0U;
    RCC_ClearFlag();
    return by_iwdg;
}

#endif /* WATCHDOG_H */
