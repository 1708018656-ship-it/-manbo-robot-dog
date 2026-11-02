/* Host tests for Battery.h: ADC conversion, averaging and hysteresis. */
#include <stdio.h>
#include "Battery.h"

static int failures = 0, assertions = 0;
#define CHECK(c) do { assertions++; if (!(c)) { failures++; printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)

/* ADC code that corresponds to a battery voltage (inverse of the divider). */
static uint16_t adc_for_mv(uint32_t mv)
{
    return (uint16_t)((mv * BATTERY_DIV_BOTTOM_KOHM * BATTERY_ADC_FULL_SCALE) /
                      ((BATTERY_DIV_TOP_KOHM + BATTERY_DIV_BOTTOM_KOHM) * BATTERY_VREF_MV));
}

static BatteryLevel feed(BatteryMonitor *m, uint32_t mv, int n)
{
    BatteryLevel l = m->level;
    while (n-- > 0) { l = Battery_AddSample(m, adc_for_mv(mv)); }
    return l;
}

int main(void)
{
    BatteryMonitor m;
    uint16_t mv;

    /* Conversion: full scale = 3.3 V * 133/33 = 13.30 V */
    CHECK(Battery_AdcToMillivolts(0) == 0);
    mv = Battery_AdcToMillivolts(4095);
    CHECK(mv >= 13299 && mv <= 13301);
    mv = Battery_AdcToMillivolts(adc_for_mv(8400));
    CHECK(mv >= 8390 && mv <= 8400);           /* within 1 LSB (~3.2 mV) */

    /* Needs a full window before reporting */
    Battery_Init(&m);
    CHECK(feed(&m, 8000, BATTERY_AVG_COUNT - 1) == BATTERY_UNKNOWN);
    CHECK(feed(&m, 8000, 1) == BATTERY_OK);

    /* Drop to low, then critical */
    CHECK(feed(&m, 6700, BATTERY_AVG_COUNT) == BATTERY_LOW);
    CHECK(feed(&m, 6300, BATTERY_AVG_COUNT) == BATTERY_CRITICAL);

    /* Hysteresis: recovering just above the critical threshold stays critical */
    CHECK(feed(&m, 6500, BATTERY_AVG_COUNT) == BATTERY_CRITICAL);
    CHECK(feed(&m, 6650, BATTERY_AVG_COUNT) == BATTERY_LOW);
    CHECK(feed(&m, 6900, BATTERY_AVG_COUNT) == BATTERY_LOW);      /* < 6800+200 */
    CHECK(feed(&m, 7100, BATTERY_AVG_COUNT) == BATTERY_OK);

    /* A single servo-stall dip does not trip critical thanks to averaging */
    Battery_Init(&m);
    feed(&m, 7400, BATTERY_AVG_COUNT);
    CHECK(Battery_AddSample(&m, adc_for_mv(5000)) == BATTERY_OK);

    /* No battery / USB-powered: unknown, never critical */
    Battery_Init(&m);
    CHECK(feed(&m, 300, BATTERY_AVG_COUNT) == BATTERY_UNKNOWN);

    printf("battery_tests: %d assertions, %d failures\n", assertions, failures);
    return failures != 0;
}
