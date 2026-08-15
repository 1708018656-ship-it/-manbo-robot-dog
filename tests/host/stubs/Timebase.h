#ifndef TEST_TIMEBASE_H
#define TEST_TIMEBASE_H

#include <stdint.h>

uint8_t Timebase_PeriodicDue(uint32_t now_ms,
                             uint32_t *next_deadline_ms,
                             uint32_t period_ms);

#endif
