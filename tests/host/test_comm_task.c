/* Host tests for CommTask: no lost commands, BT priority, '5' as e-stop. */
#include <stdio.h>
#include <string.h>
#include "CommTask.h"

static const char *g_bt = "", *g_asr = "";
static unsigned g_stops, g_faults;
static int failures = 0, assertions = 0;
#define CHECK(c) do { assertions++; if (!(c)) { failures++; printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)

uint8_t USART1_ReadByte(uint8_t *v) { if (!*g_bt) return 0; *v = (uint8_t)*g_bt++; return 1; }
uint8_t USART3_ReadByte(uint8_t *v) { if (!*g_asr) return 0; *v = (uint8_t)*g_asr++; return 1; }
uint8_t USART1_RxOverflowed(void) { return 0; }
uint8_t USART3_RxOverflowed(void) { return 0; }
void SafetyTask_RequestEmergencyStop(void) { g_stops++; }
void SafetyTask_RequestCommunicationFault(void) { g_faults++; }

static void drain(char *out)
{
    uint8_t c; int n = 0;
    while (CommTask_PopCommand(&c)) out[n++] = (char)c;
    out[n] = 0;
}

int main(void)
{
    char got[64];
    uint32_t t;

    /* two commands arriving between loop iterations are both kept, in order */
    CommTask_Init(); g_bt = "xf"; g_asr = "";
    CommTask_Run(0);
    drain(got); CHECK(strcmp(got, "xf") == 0);

    /* Bluetooth before voice, '5' becomes an emergency stop, CR/LF dropped */
    CommTask_Init(); g_stops = 0; g_bt = "d\r\n"; g_asr = "o5b";
    CommTask_Run(0);
    drain(got); CHECK(strcmp(got, "dob") == 0); CHECK(g_stops == 1);

    /* bursts longer than 4 bytes are spread over several runs, nothing lost */
    CommTask_Init(); g_bt = "fblrwzdo"; g_asr = "";
    for (t = 0; t < 3; ++t) CommTask_Run(t);
    drain(got); CHECK(strcmp(got, "fblrwzdo") == 0);

    CHECK(CommTask_ClearCommands() == 0);
    printf("comm_task_tests: %d assertions, %d failures\n", assertions, failures);
    return failures != 0;
}
