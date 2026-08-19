#ifndef PERF_PROBE_H
#define PERF_PROBE_H

#include <stdint.h>

/* Set to 1 only in a dedicated measurement build. */
#ifndef PERF_PROBE_ENABLE
#define PERF_PROBE_ENABLE 0
#endif

/*
 * Lightweight on-target task timing for Cortex-M3 firmware.
 * Results are raw core-cycle counts and must be converted with the actual
 * SystemCoreClock configured by the flashed build.
 */
typedef enum
{
    PERF_PROBE_COMM = 0,
    PERF_PROBE_SAFETY,
    PERF_PROBE_SENSOR,
    PERF_PROBE_DECISION,
    PERF_PROBE_ACTION,
    PERF_PROBE_AUDIO,
    PERF_PROBE_UI,
    PERF_PROBE_TASK_COUNT
} PerfProbeTaskId;

typedef struct
{
    uint32_t sample_count;
    uint32_t total_cycles;
    uint32_t max_cycles;
} PerfProbeStats;

void PerfProbe_Init(void);
uint8_t PerfProbe_IsEnabled(void);
uint32_t PerfProbe_Begin(void);
void PerfProbe_End(PerfProbeTaskId task_id, uint32_t start_cycles);
void PerfProbe_Reset(void);
const PerfProbeStats *PerfProbe_Get(PerfProbeTaskId task_id);

#endif /* PERF_PROBE_H */
