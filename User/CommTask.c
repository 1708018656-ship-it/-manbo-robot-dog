#include "CommTask.h"
#include "usart1.h"
#include "usart3.h"
#include "SafetyTask.h"
#include "ByteRing.h"

#define COMM_MAX_BYTES_PER_RUN 4U
#define COMMAND_EMERGENCY_STOP ((uint8_t)'5')

/*
 * Every received byte goes through the UART ring buffers into these command
 * FIFOs. Previously main() read the single-byte "last received" variable,
 * so a byte arriving before the loop got round to it (e.g. an app sending
 * 'x' then 'f') was silently overwritten.
 * Bluetooth keeps priority over voice: its FIFO is always drained first.
 */
static ByteRing g_bt_cmds;
static ByteRing g_asr_cmds;

static void CommTask_ProcessByte(ByteRing *fifo, uint8_t byte)
{
    if (byte == COMMAND_EMERGENCY_STOP)
    {
        /* main() sees the stop through SafetyTask_ConsumeStopRequest() */
        SafetyTask_RequestEmergencyStop();
        return;
    }
    if ((byte == 0U) || (byte == '\r') || (byte == '\n'))
    {
        return;     /* terminal apps often append line endings */
    }
    ByteRing_PushFromIsr(fifo, byte);   /* same context as the pop: no race */
}

void CommTask_Init(void)
{
    /* USART 的硬件初始化仍由 main.c 负责；这里只初始化命令队列。 */
    ByteRing_Init(&g_bt_cmds);
    ByteRing_Init(&g_asr_cmds);
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
        CommTask_ProcessByte(&g_bt_cmds, byte);
        processed++;
    }

    processed = 0U;
    while ((processed < COMM_MAX_BYTES_PER_RUN) &&
           (USART3_ReadByte(&byte) != 0U))
    {
        CommTask_ProcessByte(&g_asr_cmds, byte);
        processed++;
    }

    if ((USART1_RxOverflowed() != 0U) ||
        (USART3_RxOverflowed() != 0U))
    {
        SafetyTask_RequestCommunicationFault();
    }
}

uint8_t CommTask_PopCommand(uint8_t *command)
{
    if (ByteRing_Pop(&g_bt_cmds, command) != 0U)
    {
        return 1U;
    }
    return ByteRing_Pop(&g_asr_cmds, command);
}

uint8_t CommTask_ClearCommands(void)
{
    uint8_t dummy;
    uint8_t n = 0U;
    while (CommTask_PopCommand(&dummy) != 0U) { n++; }
    return n;
}
