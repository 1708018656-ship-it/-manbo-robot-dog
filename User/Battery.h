#ifndef BATTERY_H
#define BATTERY_H

/*
 * Battery monitor logic (pure, host-testable). Hardware access lives in
 * BatteryTask.c.
 *
 * Board V2 divider: VBAT -- 100k -- PA4 -- 33k -- GND, 100 nF at PA4.
 *   V_adc = VBAT * 33 / 133  -> 8.4 V (2S full) = 2.08 V, max 13.3 V.
 *
 * Thresholds for a 2S Li-ion / LiPo pack, with hysteresis so the state
 * does not chatter while servos pull the voltage down under load.
 */

#include <stdint.h>

#define BATTERY_ADC_FULL_SCALE  4095U
#define BATTERY_VREF_MV         3300U
#define BATTERY_DIV_TOP_KOHM    100U
#define BATTERY_DIV_BOTTOM_KOHM 33U

#define BATTERY_LOW_MV          6800U   /* 3.40 V/cell: warn          */
#define BATTERY_CRITICAL_MV     6400U   /* 3.20 V/cell: lie down, stop */
#define BATTERY_HYSTERESIS_MV    200U
#define BATTERY_NO_BATTERY_MV   1000U   /* below this: USB-powered / no divider */

#define BATTERY_AVG_SHIFT 3U            /* average of 8 samples */
#define BATTERY_AVG_COUNT (1U << BATTERY_AVG_SHIFT)

typedef enum
{
    BATTERY_UNKNOWN = 0,   /* not enough samples yet, or no battery sensed */
    BATTERY_OK,
    BATTERY_LOW,
    BATTERY_CRITICAL
} BatteryLevel;

typedef struct
{
    uint16_t samples[BATTERY_AVG_COUNT];
    uint32_t sum;
    uint8_t index;
    uint8_t count;
    uint16_t mv;
    BatteryLevel level;
} BatteryMonitor;

static inline uint16_t Battery_AdcToMillivolts(uint16_t adc)
{
    /* max 4095 * 3300 * 133 = 1.80e9 < 2^32 */
    uint32_t num = (uint32_t)adc * BATTERY_VREF_MV *
                   (BATTERY_DIV_TOP_KOHM + BATTERY_DIV_BOTTOM_KOHM);
    uint32_t den = (uint32_t)BATTERY_ADC_FULL_SCALE * BATTERY_DIV_BOTTOM_KOHM;
    return (uint16_t)((num + den / 2U) / den);
}

static inline void Battery_Init(BatteryMonitor *m)
{
    uint8_t i;
    for (i = 0U; i < BATTERY_AVG_COUNT; ++i) { m->samples[i] = 0U; }
    m->sum = 0U;
    m->index = 0U;
    m->count = 0U;
    m->mv = 0U;
    m->level = BATTERY_UNKNOWN;
}

static inline BatteryLevel Battery_Classify(BatteryLevel prev, uint16_t mv)
{
    if (mv < BATTERY_NO_BATTERY_MV)
    {
        return BATTERY_UNKNOWN;
    }
    switch (prev)
    {
        case BATTERY_CRITICAL:
            if (mv >= BATTERY_LOW_MV + BATTERY_HYSTERESIS_MV) { return BATTERY_OK; }
            if (mv >= BATTERY_CRITICAL_MV + BATTERY_HYSTERESIS_MV) { return BATTERY_LOW; }
            return BATTERY_CRITICAL;
        case BATTERY_LOW:
            if (mv < BATTERY_CRITICAL_MV) { return BATTERY_CRITICAL; }
            if (mv >= BATTERY_LOW_MV + BATTERY_HYSTERESIS_MV) { return BATTERY_OK; }
            return BATTERY_LOW;
        default:
            if (mv < BATTERY_CRITICAL_MV) { return BATTERY_CRITICAL; }
            if (mv < BATTERY_LOW_MV) { return BATTERY_LOW; }
            return BATTERY_OK;
    }
}

/* Feed one raw ADC sample; returns the (possibly new) level. */
static inline BatteryLevel Battery_AddSample(BatteryMonitor *m, uint16_t adc)
{
    m->sum -= m->samples[m->index];
    m->samples[m->index] = adc;
    m->sum += adc;
    m->index = (uint8_t)((m->index + 1U) & (BATTERY_AVG_COUNT - 1U));
    if (m->count < BATTERY_AVG_COUNT)
    {
        m->count++;
        if (m->count < BATTERY_AVG_COUNT)
        {
            return m->level;        /* wait until the window is full */
        }
    }
    m->mv = Battery_AdcToMillivolts((uint16_t)(m->sum >> BATTERY_AVG_SHIFT));
    m->level = Battery_Classify(m->level, m->mv);
    return m->level;
}

#endif /* BATTERY_H */
