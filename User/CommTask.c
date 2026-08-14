#include "CommTask.h"
#include "usart1.h"
#include "usart3.h"
#include "SafetyTask.h"

#define COMM_MAX_BYTES_PER_RUN 4U
#define COMMAND_EMERGENCY_STOP ((uint8_t)'5')

static void CommTask_ProcessByte(uint8_t byte)
{
    if (byte == COMMAND_EMERGENCY_STOP)
    {
        SafetyTask_RequestEmergencyStop();
    }
}

void CommTask_Init(void)
{
    /* USART 的硬件初始化仍由 main.c 负责；这里仅保留任务初始化入口。 */
}

void CommTask_Run(uint32_t now_ms)
{
    uint8_t byte;
    uint8_t processed;

    (void)now_ms;
    processed = 0U;

    while ((processed < COMM_MAX_BYTES_PER_RUN) &&
           (USART1_ReadByte(&byte) != 0U))
    {
        CommTask_ProcessByte(byte);
        processed++;
    }

    processed = 0U;
    while ((processed < COMM_MAX_BYTES_PER_RUN) &&
           (USART3_ReadByte(&byte) != 0U))
    {
        CommTask_ProcessByte(byte);
        processed++;
    }

    if ((USART1_RxOverflowed() != 0U) ||
        (USART3_RxOverflowed() != 0U))
    {
        SafetyTask_RequestCommunicationFault();
    }
}
