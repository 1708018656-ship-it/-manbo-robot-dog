/* Host tests for SensorTask ultrasonic ranging (non-blocking, 10 ms polling). */
#include <stdio.h>
#include "SensorTask.h"
#include "ActionTask.h"

uint8_t g_echo_level;
unsigned g_trigger_count;
volatile uint16_t Time;          /* 100 us ticks, counted by TIM2 ISR while Echo high */
static ActionId g_requested = ACTION_NONE;
static int failures = 0, assertions = 0;
#define CHECK(c) do { assertions++; if (!(c)) { failures++; printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)

void Timer_Init(void) { Time = 0U; }
void Timer_Stop(void) { }
uint8_t ActionTask_RequestMotion(ActionId a) { g_requested = a; return 1U; }
uint8_t ActionTask_IsActive(void) { return 0U; }

/*
 * Simulate one measurement with 0.1 ms resolution: echo rises rise_ms after
 * the trigger and stays high for width_ms. SensorTask_Run is only called
 * every 10 ms, like AppTask does.
 */
static void measure(uint32_t *now, double rise_ms, double width_ms)
{
    uint32_t t0 = *now;
    uint32_t tick;
    for (tick = 0; tick < 400U; ++tick)       /* 40 ms in 0.1 ms steps (one window) */
    {
        double t = tick / 10.0;
        uint32_t ms = t0 + tick / 10U;
        g_echo_level = (g_trigger_count > 0U && t >= rise_ms && t < rise_ms + width_ms) ? 1U : 0U;
        if (g_echo_level) { Time++; }         /* what the TIM2 ISR does */
        if ((tick % 100U) == 0U) { SensorTask_Run(ms); }   /* every 10 ms */
    }
    *now = t0 + 100U;
}

int main(void)
{
    uint32_t now = 0U;

    SensorTask_Init();
    SensorTask_SetEnabled(1U);

    /* 100 cm -> echo width = 100 * 2 / 0.0343 us = 5.83 ms */
    g_trigger_count = 0U;
    SensorTask_Run(now);                      /* trigger at t=0 */
    CHECK(g_trigger_count == 1U);
    measure(&now, 0.5, 5.83);
    CHECK(SensorTask_IsValid() == 1U);
    printf("  100 cm target measured as %u cm\n", SensorTask_LastDistanceCm());
    CHECK(SensorTask_LastDistanceCm() >= 97U && SensorTask_LastDistanceCm() <= 103U);
    CHECK(g_requested == ACTION_FORWARD);     /* no obstacle -> keep walking */

    /* 10 cm obstacle -> width 0.58 ms -> turn right */
    g_trigger_count = 0U; g_requested = ACTION_NONE;
    SensorTask_Run(now);
    measure(&now, 0.5, 0.58);
    CHECK(SensorTask_IsValid() == 1U);
    CHECK(SensorTask_LastDistanceCm() <= 12U);
    CHECK(g_requested == ACTION_TURN_RIGHT);

    /* No echo at all -> invalid, no motion requested */
    g_trigger_count = 0U; g_requested = ACTION_NONE;
    SensorTask_Run(now);
    measure(&now, 1000.0, 0.0);
    CHECK(SensorTask_IsValid() == 0U);
    CHECK(g_requested == ACTION_NONE);

    printf("sensor_task_tests: %d assertions, %d failures\n", assertions, failures);
    return failures != 0;
}
