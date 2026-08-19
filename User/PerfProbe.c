#include "PerfProbe.h"

#if PERF_PROBE_ENABLE

/* Cortex-M3 CoreSight register addresses. */
#define PERF_PROBE_DEMCR         (*(volatile uint32_t *)0xE000EDFCUL)
#define PERF_PROBE_DWT_CTRL      (*(volatile uint32_t *)0xE0001000UL)
#define PERF_PROBE_DWT_CYCCNT    (*(volatile uint32_t *)0xE0001004UL)
#define PERF_PROBE_DEMCR_TRCENA  (1UL << 24)
#define PERF_PROBE_DWT_CYCCNTENA (1UL << 0)

static PerfProbeStats g_stats[PERF_PROBE_TASK_COUNT];
static uint8_t g_enabled = 0U;

void PerfProbe_Init(void)
{
    PerfProbe_Reset();

    PERF_PROBE_DEMCR |= PERF_PROBE_DEMCR_TRCENA;
    PERF_PROBE_DWT_CYCCNT = 0UL;
    PERF_PROBE_DWT_CTRL |= PERF_PROBE_DWT_CYCCNTENA;

    g_enabled = ((PERF_PROBE_DWT_CTRL & PERF_PROBE_DWT_CYCCNTENA) != 0UL) ? 1U : 0U;
}

uint8_t PerfProbe_IsEnabled(void)
{
    return g_enabled;
}

uint32_t PerfProbe_Begin(void)
{
    if (g_enabled == 0U)
    {
        return 0UL;
    }

    return PERF_PROBE_DWT_CYCCNT;
}

void PerfProbe_End(PerfProbeTaskId task_id, uint32_t start_cycles)
{
    uint32_t elapsed_cycles;
    PerfProbeStats *stats;

    if ((g_enabled == 0U) || ((uint32_t)task_id >= (uint32_t)PERF_PROBE_TASK_COUNT))
    {
        return;
    }

    /* Unsigned subtraction remains valid across a single 32-bit counter wrap. */
    elapsed_cycles = PERF_PROBE_DWT_CYCCNT - start_cycles;
    stats = &g_stats[(uint32_t)task_id];

    stats->sample_count++;
    stats->total_cycles += elapsed_cycles;
    if (elapsed_cycles > stats->max_cycles)
    {
        stats->max_cycles = elapsed_cycles;
    }
}

void PerfProbe_Reset(void)
{
    uint32_t i;

    for (i = 0UL; i < (uint32_t)PERF_PROBE_TASK_COUNT; ++i)
    {
        g_stats[i].sample_count = 0UL;
        g_stats[i].total_cycles = 0UL;
        g_stats[i].max_cycles = 0UL;
    }
}

const PerfProbeStats *PerfProbe_Get(PerfProbeTaskId task_id)
{
    if ((uint32_t)task_id >= (uint32_t)PERF_PROBE_TASK_COUNT)
    {
        return (const PerfProbeStats *)0;
    }

    return &g_stats[(uint32_t)task_id];
}

#else

void PerfProbe_Init(void) {}
uint8_t PerfProbe_IsEnabled(void) { return 0U; }
uint32_t PerfProbe_Begin(void) { return 0UL; }
void PerfProbe_End(PerfProbeTaskId task_id, uint32_t start_cycles)
{
    (void)task_id;
    (void)start_cycles;
}
void PerfProbe_Reset(void) {}
const PerfProbeStats *PerfProbe_Get(PerfProbeTaskId task_id)
{
    (void)task_id;
    return (const PerfProbeStats *)0;
}

#endif /* PERF_PROBE_ENABLE */
