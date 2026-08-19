# STM32 板上性能测量协议

**适用工程：** `Project.uvprojx`，目标为 STM32F103C8（Cortex-M3）。  
**目的：** 用真实板卡数据验证协作式调度器的响应时间、动作任务开销和资源余量；不以主机侧估算代替实测。

## 已接入的测量能力

`User/PerfProbe.c` 使用 Cortex-M3 的 DWT `CYCCNT` 计数器记录每个调度任务的周期数。为避免诊断能力侵占量产资源，`PERF_PROBE_ENABLE` 默认值为 `0`：量产构建不创建统计数组，也不会在调度器中调用采样函数。仅在专用测量构建把该宏设为 `1` 后，`AppTask_Run()` 才会为通信、安全、传感器、决策、动作、音频和 UI 任务分别累计调用次数、总周期数和最大周期数。

| 枚举 | 调度任务 | 计划周期 | 关注指标 |
|---|---|---:|---|
| `PERF_PROBE_COMM` | `CommTask_Run` | 1 ms | 串口突发下的单次最大周期数。 |
| `PERF_PROBE_SAFETY` | `SafetyTask_Run` | 1 ms | 急停/故障路径的最大周期数。 |
| `PERF_PROBE_SENSOR` | `SensorTask_Run` | 10 ms | 测距或红外采样的长尾开销。 |
| `PERF_PROBE_DECISION` | `DecisionTask` | 1 ms | 状态转换的时间预算。 |
| `PERF_PROBE_ACTION` | `ActionTask_Run` | 1 ms | 姿态更新与 PWM 写入成本。 |
| `PERF_PROBE_AUDIO` | `AudioTask_Run` | 10 ms | 语音帧发送节奏。 |
| `PERF_PROBE_UI` | `UiTask` | 50 ms | OLED/LED 刷新的峰值开销。 |

## 使用步骤

1. 在 Keil MDK 中重新打开 `Project.uvprojx`，确认 `User/PerfProbe.c` 与 `User/PerfProbe.h` 已被登记到 **User** 组。
2. 复制一个**测量构建**配置，在 C/C++ 宏定义中加入 `PERF_PROBE_ENABLE=1`；量产构建保持默认 `0`。这确保性能诊断不会以常驻 RAM、Flash 或调度调用开销换取便利。
3. 使用与实际目标板一致的优化级别构建并烧录测量构建。`PerfProbe_Init()` 会在 `AppTask_Init()` 内自动启用 DWT 周期计数器。
4. 在调试器的 Watch 窗口调用 `PerfProbe_IsEnabled()`，确认返回 `1`。若返回 `0`，说明目标/调试配置未允许 DWT 计数，不能把后续统计当作有效数据。
5. 运行预定义场景至少 60 秒：静止待机、连续蓝牙控制、连续语音命令、超声波避障、复杂动作和 OLED 高频刷新。
6. 对每个任务 ID 调用 `PerfProbe_Get(id)`，记录 `sample_count`、`total_cycles` 和 `max_cycles`。平均周期数为 `total_cycles / sample_count`；在 72 MHz 下，微秒数约为 `cycles / 72`。
7. 使用同一次构建产生的 Keil `.map` 文件记录 `Code`、`RO-data`、`RW-data` 和 `ZI-data`，并把资源数据与周期数据一起归档。

## 建议的验收记录

| 场景 | 任务 | 平均周期 | 最大周期 | 目标 | 结论 |
|---|---|---:|---:|---:|---|
| 静止待机 | Safety | 待测 | 待测 | < 50 µs | 待填写 |
| 蓝牙突发 | Comm | 待测 | 待测 | < 200 µs | 待填写 |
| 连续动作 | Action | 待测 | 待测 | < 100 µs | 待填写 |
| 避障 | Sensor | 待测 | 待测 | 按测距方案定义 | 待填写 |
| OLED 刷新 | UI | 待测 | 待测 | 不影响安全周期 | 待填写 |

> 这些目标是验收阈值，不是当前实测结论。若旧的阻塞动作、忙等串口或无界超声波循环仍在运行，任何“< 5 ms 急停响应”都不能被视为已验证。

## 限制与下一步

DWT 采样只测量被协作式调度器包装的任务函数，不能自动覆盖中断服务例程、旧阻塞动作或未接入的裸循环。下一步应将关键 ISR 的频率与执行时间通过 GPIO 或逻辑分析仪交叉验证，并在迁移完阻塞路径后，提交包含测试场景、最大周期、固件 SHA 和 `.map` 摘要的性能基线报告。
