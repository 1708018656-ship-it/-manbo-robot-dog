#include "SafetyTask.h"

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

    /*
     * 第一阶段兼容策略：先锁存安全请求，具体舵机安全姿态由现有
     * main.c/Mode.c 在消费请求后执行。后续动作状态机接入后，
     * 这里将直接调用 ActionTask_AbortToSafeStop()。
     */
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
