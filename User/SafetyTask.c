#include "SafetyTask.h"
#include "ActionTask.h"

#define SAFETY_FAULT_COMM_OVERFLOW 0x01U

static volatile uint8_t g_stop_request = 0U;
static volatile uint8_t g_fault_flags = 0U;

void SafetyTask_Init(void)
{
    g_stop_request = 0U;
    g_fault_flags = 0U;
}

void SafetyTask_RequestEmergencyStop(void)
{
    g_stop_request = 1U;
}

void SafetyTask_RequestCommunicationFault(void)
{
    g_fault_flags |= SAFETY_FAULT_COMM_OVERFLOW;
    g_stop_request = 1U;
}

void SafetyTask_Run(uint32_t now_ms)
{
    (void)now_ms;

    if (g_stop_request != 0U)
    {
        ActionTask_AbortToSafeStop();
        g_stop_request = 0U;
    }
}

uint8_t SafetyTask_ConsumeStopRequest(void)
{
    uint8_t requested;

    requested = g_stop_request;
    g_stop_request = 0U;
    return requested;
}

uint8_t SafetyTask_IsFaultActive(void)
{
    return (g_fault_flags != 0U) ? 1U : 0U;
}
