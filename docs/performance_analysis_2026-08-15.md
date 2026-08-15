# 曼波机器狗 STM32 性能与资源分析报告

**分析日期：** 2026-08-15
**目标工程：** `Project.uvprojx`，`STM32F103C8`、Cortex-M3  72 MHz 级别目标配置，工程声明 IRAM `0x5000`（20 KiB）、IROM `0x10000`（64 KiB）。
**分析范围：** STM32 主控固件、PoseStep 状态机、协作式任务调度器、串口/定时器中断、OLED/舵机/超声波/SYN6288 驱动，以及主机侧测试与资源统计脚本。

> 本报告区分“源码可以精确计算的资源”和“必须在真实 STM32 上通过 Keil map 文件、DWT 周期计数器或 GPIO 示波测量的资源”。当前环境没有 Keil/ARM GCC 和目标板，因此 CPU 百分比、最终 Flash 镜像大小、真实栈峰值和中断周期数只能给出上界、公式和测量方案，不能伪造实测值。

## 一、结论摘要

当前状态机本身较轻：20 个 PoseStep 脚本共 98 个姿态步骤，按 ARM 结构体对齐估算约占 **784 字节只读数据**；运行时动作实例只保存指针、索引、截止时间和状态，约几十字节。已知 OLED 显存占 **1,024 字节 RAM**，两个串口环形缓冲区合计约 **38 字节**，音频发送帧占 **50 字节**，已识别的静态和局部缓冲小计约 **1,298 字节**，另有任务表、全局状态、OLED 字库和旧业务模块未完全计入。

真正的性能瓶颈不在 PoseStep，而在尚未完全迁移的旧阻塞路径。源码中统计到约 **140 处 `Delay_ms/us/s` 调用**，包括 `User/Mode.c` 和 `Hardware/Movement.c`；主循环中仍存在 `Delay_ms(100)` 的超声波测距和无界避障循环。只要旧动作或 `Bizhang()` 被调用，1 ms 的通信、安全、动作任务调度就可能被长时间阻塞，导致急停响应延迟远高于状态机的理论 1 ms 调度周期。

中断方面，TIM4 每 1 ms 触发一次时基中断；超声波计时器 TIM2 配置为约 100 µs 更新周期，在测量期间理论上可达到 **10,000 次/秒** 的中断频率；115200 bps 串口在最坏连续接收时约为 **11,520 字节/秒**，按 8N1 近似每字节一次接收中断。TIM2 和高波特率串口是需要用 DWT/示波器实测的主要 CPU 风险。

## 二、目标资源边界与分析方法

### 2.1 工程资源边界

| 项目 | 工程配置或源码结果 |
|---|---:|
| MCU | STM32F103C8，Cortex-M3 |
| IRAM | 20,480 bytes |
| IROM | 65,536 bytes |
| 调度任务槽位 | 7 |
| 任务周期 | 1 ms、1 ms、10 ms、1 ms、1 ms、10 ms、50 ms |
| 状态机动作脚本 | 20 组、98 个 PoseStep |
| OLED 显示缓冲区 | 8 × 128 = 1,024 bytes |

### 2.2 计算边界

源码级 RAM 统计没有把 Keil 启动代码、C 库、全局变量的对齐填充、调用栈、链接器保留区和库内部缓冲纳入最终结果；因此“已知小计”不是最终 `.data + .bss + stack`。最终结论必须以 Keil `.map` 文件中的 `RW-data`、`ZI-data`、`Code` 和栈区配置为准。

Flash 方面，主机 GCC 对 `Hardware/OLED_Data.c` 的独立只读段测得约 **16,144 字节 `.rodata`**。该值不是 ARM 最终镜像值，但说明 OLED 字库是目前最明显的只读数据大户之一；再叠加 STM32 标准外设库、启动代码和业务代码后，需要重点确认 64 KiB IROM 是否存在余量。

## 三、静态 RAM 与 Flash 开销

### 3.1 已知 RAM 对象

| 对象 | 估算大小 | 类型与位置 | 说明 |
|---|---:|---|---|
| `OLED_DisplayBuf[8][128]` | 1,024 B | 全局 RAM，`Hardware/OLED.c` | 约占 20 KiB IRAM 的 5.0%。 |
| 两个 `ByteRing` | 38 B | 全局 RAM，USART1/3 | 每个为 16 B 数据 + 3 个字节索引/标志。 |
| AudioTask 帧 | 50 B | 全局 RAM，`User/AudioTask.c` | 非阻塞 SYN6288 当前帧。 |
| Timebase 计数器 | 4 B | 全局 RAM | `uint32_t` 毫秒计数。 |
| Safety 标志 | 2 B | 全局 RAM | 急停和故障标志。 |
| AppTask 表 | 约 84 B | 全局 RAM | 7 槽位，每槽按 32 位函数指针 + 两个 32 位字段估算 12 B。 |
| `Serial_Printf` 局部缓冲 | 100 B | 栈 | 单次调用峰值；已使用 `vsnprintf`。 |
| `OLED_Printf` 局部缓冲 | 30 B | 栈 | 单次调用峰值；已使用 `vsnprintf`。 |
| SYN6288 局部帧 | 50 B | 栈 | `SYN_FrameInfo()` 调用期间存在。 |

已识别静态/局部对象小计约为 **1,298 B**，但不包括 `ActionInstance`、其他业务全局变量、OLED/字库工作变量、外设库对象和调用栈对齐开销。单次调用的栈峰值尤其需要检查 `SYN_FrameInfo()`、`OLED_Printf()`、`Serial_Printf()` 和旧动作函数的嵌套关系。

### 3.2 PoseStep 只读数据与运行时状态

`PoseStep` 由 4 个角度字节和一个 32 位持续时间组成。在 ARM 32 位 ABI 下，单个结构体通常按 8 字节对齐。当前脚本共 98 个步骤，估算为 **784 B Flash/只读数据**。各脚本如下：

| 动作脚本 | 步骤数 | 估算大小 |
|---|---:|---:|
| 站立 | 1 | 8 B |
| 前进/后退 | 16 | 128 B |
| 左转/右转 | 8 | 64 B |
| 摇摆/舞蹈 | 19 | 152 B |
| 打招呼/伸展/抬手/懒腰/抬头 | 27 | 216 B |
| 睡觉/高级组合动作 | 27 | 216 B |
| **合计** | **98** | **784 B** |

因此，继续增加动作脚本对 RAM 影响很小，主要消耗 Flash；不要把所有姿态复制到运行时 RAM。当前 `ActionInstance` 采用脚本指针而不是复制数组，是合理的低内存设计。

### 3.3 Flash 主要风险

OLED 字库独立编译得到约 16.1 KiB `.rodata`，占 64 KiB IROM 的约 24.6%。标准外设库、启动代码和 OLED/运动代码的函数体还会继续占用 IROM。建议在 Keil 中生成 map 文件，并关注以下三项：

| 指标 | 计算方式 | 建议警戒线 |
|---|---|---:|
| Code + RO-data | map 中代码和只读数据总和 | 超过 52 KiB 时开始裁剪字库/调试代码 |
| RW-data + ZI-data | map 中 RAM 初始化数据和零初始化数据 | 超过 14 KiB 时为栈和故障保留空间 |
| 最大栈深度 | 静态调用图 + 栈填充实测 | 不超过 IRAM 剩余空间的 70% |

## 四、CPU 与调度负载分析

### 4.1 协作式任务频率

当前调度表每轮扫描 7 个槽位。每次 `AppTask_Run()` 都会进行 7 次截止时间判断；到期后最多执行以下频率：

| 任务 | 周期 | 最大理论调用次数 |
|---|---:|---:|
| CommTask | 1 ms | 1,000 次/秒 |
| SafetyTask | 1 ms | 1,000 次/秒 |
| SensorTask | 10 ms | 100 次/秒 |
| DecisionTask | 1 ms | 1,000 次/秒 |
| ActionTask | 1 ms | 1,000 次/秒 |
| AudioTask | 10 ms | 100 次/秒 |
| UI Task | 50 ms | 20 次/秒 |

任务周期本身不等于 CPU 占用。粗略预算公式为：

```text
CPU 占用率 ≈ Σ(任务每次执行平均周期数 × 每秒执行次数)
             / (72,000,000 cycles/s)
           + Σ(ISR 每次周期数 × 每秒触发次数)
```

当前源码没有 DWT 周期测量，因此不能给出可信的百分比。建议先对每个任务加入 `DWT->CYCCNT` 或 GPIO 脉冲测量，再填入上述公式。

### 4.2 状态机任务的实际成本

`ActionTask_Run()` 主要包含时间比较、脚本索引推进和四次 `Servo_SetAngle*()` 调用。它不执行循环、不复制 PoseStep 数组，也不调用 `Delay_*`；在 72 MHz Cortex-M3 上，预计远低于 1 ms 周期，但实际成本会受 `PWM_SetCompare*()` 的寄存器封装影响。

`CommTask_Run()` 每次最多处理 USART1 的 4 字节和 USART3 的 4 字节，即单次最多处理 8 个字节。这个上限把通信任务的单次工作量控制在有限范围内，但当输入持续高于消费速度时，环形缓冲区会溢出并触发安全故障。建议用吞吐测试确定 `COMM_MAX_BYTES_PER_RUN` 是否需要提高或改为按时间预算消费。

`AudioTask_Run()` 每 10 ms 最多向 USART2 写入一个字节。一个最长 50 字节帧至少需要约 500 ms 才能由任务写入完成；在 9,600 bps、8N1 的真实线上，50 字节约需 52 ms 传输时间，因此当前调度间隔会显著低于 UART 可承载速度，但它避免了阻塞主循环。可以在 TXE 中断或每次任务运行时循环发送至一个小周期预算，以降低播报延迟，但必须避免重新引入无界等待。

### 4.3 仍然存在的 CPU 阻塞热点

源码统计到约 140 处 `Delay_ms/us/s` 调用和约 25 处 `while` 循环。重要热点包括：

| 热点 | 影响 |
|---|---|
| `User/Mode.c` 旧高级模式 | 语音、OLED 和动作之间使用秒级延时，阻塞所有协作式任务。 |
| `Hardware/Movement.c` | 复杂动作的平滑插值和等待循环会占满主循环。 |
| `Hardware/UltrasonicWave.c` | `Delay_ms(100)` 加上 `while(T < 15)`，可能造成百毫秒到无限长阻塞。 |
| `System/usart2.c`/`Hardware/Serial.c` | 发送函数轮询 TXE，调用期间 CPU 忙等。 |
| `Hardware/OLED.c` | 全屏更新和绘图循环可能在 50 ms UI 槽位内产生明显峰值。 |

因此，当前状态机的理论 1 ms 响应能力只有在已迁移动作路径被实际调用时才成立。只要主循环仍进入旧阻塞函数，急停延迟由阻塞函数的剩余执行时间决定，而不是由 `SafetyTask` 的 1 ms 周期决定。

## 五、中断 CPU 上界

### 5.1 TIM4 时基中断

`System/Timebase.c` 使用 TIM4 生成 1 ms 时基，因此固定触发频率为 **1,000 Hz**。中断主体只递增 `g_timebase_ms` 并清除挂起标志，理论上是低成本 ISR；仍建议测量实际入口/退出和高优先级抢占影响。

### 5.2 TIM2 超声波计时中断

`Hardware/Timer.c` 将 TIM2 配置为 `72 MHz / 7200 = 10 kHz` 更新频率，即约 **100 µs 一次**。中断期间读取 GPIO 并可能递增 `Time`。如果测距期间持续启用 TIM2，这相当于每秒约 10,000 次 ISR，远高于 TIM4。若每次 ISR 只需几十个周期，仍可能占用几个百分点；若叠加较重的库函数或临界区，可能显著影响主循环。

建议不要用 100 µs 中断持续计时整个测距周期。优先使用输入捕获、一次性超时定时器或更低频率的状态机采样。

### 5.3 USART 中断

USART1 115200 bps 在 8N1 下理论最大约 11,520 字节/秒；USART3/ASRPro 9,600 bps 约 960 字节/秒。每字节一个 RXNE ISR 时，USART1 的中断频率可能超过 11 kHz。ByteRing 入队本身很短，但应通过硬件压力测试确认在最高输入速率下不会持续溢出。

## 六、状态机资源结论

状态机设计的资源特征是 **Flash 线性增长、RAM 基本恒定、CPU 以 1 ms 调度开销为主**。增加一组包含 10 个 PoseStep 的动作，大约增加 80 字节只读数据，不增加同等规模的 RAM；但每个步骤结束时会调用四次舵机设置函数，因此动作切换瞬间的 CPU 峰值由 PWM 写寄存器路径决定。

当前最值得优化的不是压缩 `ActionInstance`，而是：

| 优化项 | 预期收益 |
|---|---|
| 将角度从 `uint8_t` 保持为整数，避免 `float` 转换 | 降低四舵机更新的指令和库开销 |
| 把 AudioTask 从 10 ms 改为 TXE 中断或小批量发送 | 降低最长 500 ms 的软件发送窗口 |
| 完成旧 `Movement.c`、`Mode.c` 和超声波路径迁移 | 消除主循环长阻塞，提升急停确定性 |
| 降低 TIM2 ISR 频率或使用输入捕获 | 减少最高约 10 kHz 的中断压力 |
| 按脏标记和局部刷新 OLED | 降低 50 ms UI 槽位的 CPU 峰值 |
| 裁剪未使用 OLED 字库和调试字符串 | 释放 Flash，保留更多固件余量 |

## 七、必须在 STM32 上执行的实测方案

### 7.1 CPU 周期测量

在调试构建中启用 Cortex-M3 的 DWT 周期计数器，在每个任务入口读取 `DWT->CYCCNT`，在退出时记录差值。每个任务至少记录平均值、P95、最大值和调用次数；不要只记录平均值，因为 OLED 刷新、动作切换和串口突发会形成长尾。

### 7.2 GPIO 示波测量

如果当前调试配置不便使用 DWT，可在每个任务外围翻转不同 GPIO。用逻辑分析仪测量高电平宽度和重复周期，得到任务执行时间、主循环空闲时间和 ISR 抢占情况。急停测试应同时观察串口 RX、SafetyTask 入口和 PWM 输出变化，获得真正的端到端延迟。

### 7.3 栈峰值测量

在启动时用固定模式填充主栈未使用区域，运行所有动作、语音、OLED、超声波和串口压力场景后扫描填充区，计算最大栈深度。重点覆盖 `Serial_Printf`、`OLED_Printf`、`SYN_FrameInfo`、旧模式函数和中断嵌套。

### 7.4 Keil map 文件

使用与目标板一致的优化级别生成 `.map`，记录 `Code`、`RO-data`、`RW-data`、`ZI-data` 和最大栈配置。将结果回填到本报告表格中，特别检查 IROM 是否接近 64 KiB、IRAM 是否为栈和中断保留足够余量。

## 八、建议验收指标

| 指标 | 建议目标 |
|---|---:|
| 纯状态机动作任务最大执行时间 | < 100 µs |
| 通信任务单次最大执行时间 | < 200 µs |
| 安全任务最大执行时间 | < 50 µs |
| 正常状态主循环最大占用 | < 50% CPU |
| TIM4 + USART + TIM2 ISR 合计长期占用 | < 20% CPU |
| 状态机急停到安全 PWM 写入 | < 5 ms（无旧阻塞路径时） |
| IRAM 使用 | < 70%，为栈和故障保留空间 |
| IROM 使用 | < 80%，保留升级和调试空间 |
| 主栈峰值 | < 配置栈容量的 70% |

这些是工程验收目标，不是当前实测结果。若旧阻塞路径仍可能被触发，应将急停指标标记为“不适用”，直到动作和传感器迁移完成。

## 九、结论

当前 PoseStep 状态机的内存开销很小，98 个步骤约 784 字节只读数据，运行时状态接近常数级；OLED 显存和字库分别是 RAM 与 Flash 的主要静态消费者。CPU 风险主要来自旧阻塞动作、超声波无界循环、约 10 kHz 的 TIM2 测量中断、连续串口 RXNE 中断和同步 OLED/USART 发送。

在没有 Keil map 文件和目标板周期测量之前，不应宣称具体 CPU 百分比或最终 RAM/Flash 使用率。下一步最有价值的工作是先补齐 DWT/GPIO 性能测量，再根据真实 P95/最大值调整任务周期、串口消费预算和 TIM2 测量方案。

## References

[1]: https://github.com/RussellCooper-DJZ/manbo-robot-dog/blob/master/Project.uvprojx "manbo-robot-dog Keil project configuration"

[2]: https://github.com/RussellCooper-DJZ/manbo-robot-dog/blob/master/User/AppTask.c "manbo-robot-dog cooperative scheduler"

[3]: https://github.com/RussellCooper-DJZ/manbo-robot-dog/blob/master/User/ActionTask.c "manbo-robot-dog PoseStep action task"

[4]: https://github.com/RussellCooper-DJZ/manbo-robot-dog/blob/master/User/AudioTask.c "manbo-robot-dog non-blocking audio task"

[5]: https://github.com/RussellCooper-DJZ/manbo-robot-dog/blob/master/Hardware/Timer.c "manbo-robot-dog ultrasonic timer ISR"

[6]: https://github.com/RussellCooper-DJZ/manbo-robot-dog/blob/master/System/Timebase.c "manbo-robot-dog TIM4 timebase"

[7]: https://github.com/RussellCooper-DJZ/manbo-robot-dog/blob/master/Hardware/OLED_Data.c "manbo-robot-dog OLED font data"

[8]: https://github.com/RussellCooper-DJZ/manbo-robot-dog/blob/master/Hardware/UltrasonicWave.c "manbo-robot-dog ultrasonic and obstacle avoidance"
