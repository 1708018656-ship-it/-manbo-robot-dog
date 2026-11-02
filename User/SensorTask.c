#include "SensorTask.h"
#include "stm32f10x.h"
#include "ActionTask.h"
#include "Timebase.h"
#include "Timer.h"
#include "Delay.h"

#define ULTRASONIC_WAIT_TIMEOUT_MS 30U
#define ULTRASONIC_PERIOD_MS       60U
#define ULTRASONIC_OBSTACLE_CM     15U
#define ULTRASONIC_TURN_COOLDOWN_MS 200U

/*
 * Ranging works on the TIM2 ISR's accumulated count, not on polled edges.
 * TIM2 fires every 100 us and increments Time only while Echo is high;
 * Timer_Init() zeroes it at trigger. This task runs only every 10 ms, so it
 * cannot see edges of echoes shorter than 10 ms (< ~170 cm): the previous
 * WAIT_RISE/WAIT_FALL polling missed most near obstacles entirely and
 * under-measured the rest. Now we simply wait one measurement window and
 * read Time, which *is* the echo width.
 */
typedef enum
{
    SENSOR_IDLE = 0,
    SENSOR_MEASURING
} SensorState;

static SensorState g_sensor_state;
static uint32_t g_deadline_ms;
static uint32_t g_next_trigger_ms;
static uint32_t g_turn_cooldown_ms;
static uint16_t g_last_distance_cm;
static uint8_t g_distance_valid;
static uint8_t g_sensor_enabled;
extern volatile uint16_t Time;

static uint8_t Sensor_DeadlineReached(uint32_t now_ms, uint32_t deadline_ms)
{
    return ((int32_t)(now_ms - deadline_ms) >= 0) ? 1U : 0U;
}

static void Sensor_Trigger(void)
{
    GPIO_SetBits(GPIOA, GPIO_Pin_0);
    /* 45 us 触发脉冲远小于任务周期，不会阻塞安全任务。 */
    Delay_us(45U);
    GPIO_ResetBits(GPIOA, GPIO_Pin_0);
    Timer_Init();
}

static void Sensor_Finish(uint32_t now_ms, uint8_t valid, uint16_t distance_cm)
{
    Timer_Stop();
    g_distance_valid = valid;
    if (valid != 0U)
    {
        g_last_distance_cm = distance_cm;
        if (distance_cm < ULTRASONIC_OBSTACLE_CM)
        {
            if (Sensor_DeadlineReached(now_ms, g_turn_cooldown_ms) != 0U)
            {
                (void)ActionTask_RequestMotion(ACTION_TURN_RIGHT);
                g_turn_cooldown_ms = now_ms + ULTRASONIC_TURN_COOLDOWN_MS;
            }
        }
        else if (ActionTask_IsActive() == 0U)
        {
            (void)ActionTask_RequestMotion(ACTION_FORWARD);
        }
    }
    g_sensor_state = SENSOR_IDLE;
    g_next_trigger_ms = now_ms + ULTRASONIC_PERIOD_MS;
}

void SensorTask_Init(void)
{
    g_sensor_state = SENSOR_IDLE;
    g_deadline_ms = 0U;
    g_next_trigger_ms = 0U;
    g_turn_cooldown_ms = 0U;
    g_last_distance_cm = 0U;
    g_distance_valid = 0U;
    g_sensor_enabled = 0U;
}

void SensorTask_SetEnabled(uint8_t enabled)
{
    g_sensor_enabled = (enabled != 0U) ? 1U : 0U;
    if (g_sensor_enabled == 0U)
    {
        g_sensor_state = SENSOR_IDLE;
        g_distance_valid = 0U;
    }
}

void SensorTask_Run(uint32_t now_ms)
{
    uint16_t elapsed_ticks;

    if (g_sensor_enabled == 0U)
    {
        return;
    }

    switch (g_sensor_state)
    {
        case SENSOR_IDLE:
            if (Sensor_DeadlineReached(now_ms, g_next_trigger_ms) != 0U)
            {
                Sensor_Trigger();
                g_deadline_ms = now_ms + ULTRASONIC_WAIT_TIMEOUT_MS;
                g_sensor_state = SENSOR_MEASURING;
            }
            break;

        case SENSOR_MEASURING:
            if (Sensor_DeadlineReached(now_ms, g_deadline_ms) == 0U)
            {
                break;
            }
            elapsed_ticks = Time;
            if ((GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1) != RESET) || (elapsed_ticks == 0U))
            {
                /* still high: no echo (HC-SR04 holds ~38 ms) / out of range;
                 * zero: sensor missing or Echo not wired */
                Sensor_Finish(now_ms, 0U, 0U);
            }
            else
            {
                /* 100 us tick * 343 m/s / 2 = 1.715 cm per tick */
                Sensor_Finish(now_ms, 1U, (uint16_t)(((uint32_t)elapsed_ticks * 1715U + 500U) / 1000U));
            }
            break;

        default:
            Sensor_Finish(now_ms, 0U, 0U);
            break;
    }
}

uint8_t SensorTask_IsValid(void)
{
    return g_distance_valid;
}

uint16_t SensorTask_LastDistanceCm(void)
{
    return g_last_distance_cm;
}
