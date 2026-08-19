#include "AppTask.h"
#include "Timebase.h"
#include "CommTask.h"
#include "SafetyTask.h"
#include "ActionTask.h"
#include "AudioTask.h"
#include "SensorTask.h"
#include "PerfProbe.h"

#define APP_TASK_COUNT 7U

typedef void (*AppTaskCallback)(uint32_t now_ms);

typedef struct
{
    AppTaskCallback callback;
    uint32_t period_ms;
    uint32_t next_deadline_ms;
} AppTaskSlot;

/*
 * 第二阶段接入通信和安全任务；动作、传感器、音频和 UI 仍保持空槽位。
 */
static void AppTask_Comm(uint32_t now_ms);
static void AppTask_Safety(uint32_t now_ms);
static void AppTask_Sensor(uint32_t now_ms);
static void AppTask_Decision(uint32_t now_ms);
static void AppTask_Action(uint32_t now_ms);
static void AppTask_Audio(uint32_t now_ms);
static void AppTask_Ui(uint32_t now_ms);

static AppTaskSlot g_tasks[APP_TASK_COUNT] =
{
    { AppTask_Comm,     1U, 0U },
    { AppTask_Safety,   1U, 0U },
    { AppTask_Sensor,  10U, 0U },
    { AppTask_Decision, 1U, 0U },
    { AppTask_Action,   1U, 0U },
    { AppTask_Audio,   10U, 0U },
    { AppTask_Ui,      50U, 0U }
};

void AppTask_Init(void)
{
    uint8_t i;
    uint32_t now_ms = Timebase_NowMs();

    SensorTask_Init();
#if PERF_PROBE_ENABLE
    PerfProbe_Init();
#endif

    for (i = 0U; i < APP_TASK_COUNT; ++i)
    {
        g_tasks[i].next_deadline_ms = now_ms + g_tasks[i].period_ms;
    }
}

void AppTask_Run(uint32_t now_ms)
{
    uint8_t i;

    for (i = 0U; i < APP_TASK_COUNT; ++i)
    {
        if (Timebase_PeriodicDue(now_ms,
                                 &g_tasks[i].next_deadline_ms,
                                 g_tasks[i].period_ms) != 0U)
        {
#if PERF_PROBE_ENABLE
            uint32_t start_cycles = PerfProbe_Begin();
            g_tasks[i].callback(now_ms);
            PerfProbe_End((PerfProbeTaskId)i, start_cycles);
#else
            g_tasks[i].callback(now_ms);
#endif
        }
    }
}

static void AppTask_Comm(uint32_t now_ms)
{
    CommTask_Run(now_ms);
}

static void AppTask_Safety(uint32_t now_ms)
{
    SafetyTask_Run(now_ms);
}

static void AppTask_Sensor(uint32_t now_ms)
{
    SensorTask_Run(now_ms);
}

static void AppTask_Decision(uint32_t now_ms)
{
    (void)now_ms;
    /* TODO(phase-2): 根据事件和优先级执行状态转换。 */
}

static void AppTask_Action(uint32_t now_ms)
{
    ActionTask_Run(now_ms);
}

static void AppTask_Audio(uint32_t now_ms)
{
    AudioTask_Run(now_ms);
}

static void AppTask_Ui(uint32_t now_ms)
{
    (void)now_ms;
    /* TODO(phase-3): 使用脏标记限频刷新 OLED 和 LED。 */
}
