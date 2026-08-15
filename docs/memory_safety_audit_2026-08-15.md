# 曼波机器狗代码库静态代码与内存安全审查报告

**审查日期：** 2026-08-15
**审查范围：** `User/`、`System/`、`Hardware/`、`ASRPro_Code/src/`、`ASRPro_Code/include/`、`ASRPro_Code/examples/`，以及当前 Keil 和 PlatformIO 构建入口。
**审查方式：** 源码模式扫描、关键函数逐行复核、主机测试套件执行和工程配置检查。

> 本报告区分“已确认的内存安全问题”“高概率未定义行为/资源风险”和“需要硬件验证的可用性风险”。没有发现 `malloc/calloc/realloc/free` 动态内存使用，因此没有观察到传统意义上的堆内存泄漏；当前主要风险来自固定栈缓冲区、未经校验的指针、数组索引和阻塞式硬件循环。

## 一、结论摘要

初始审查发现 **2 项严重的确定性栈缓冲区溢出风险**；本次修复已将两处无界 `vsprintf()` 替换为带容量限制的 `vsnprintf()`。当前仍需优先处理超声波无界阻塞、SYN6288 未终止字符串和 OLED 参数边界等风险。

| 编号 | 严重程度 | 文件与位置 | 问题 | 是否确认可触发 |
|---|---|---|---|---|
| M-01 | **已修复** | `Hardware/Serial.c:192-216` | 已改为 `vsnprintf()`，固定 100 字节缓冲区，返回值可检测截断 | 原风险已消除 |
| M-02 | **已修复** | `Hardware/OLED.c:854-873` | 已改为 `vsnprintf()`，固定 30 字节缓冲区，返回值可检测截断 | 原风险已消除 |
| M-03 | 高 | `Hardware/syn6288.c:24-27, 63-73` | 对外部指针直接 `strlen()`；未以 NUL 结尾的输入会越界读取 | 是，API 输入不可信时 |
| M-04 | 高 | `Hardware/OLED.c:553-564` | `Char - ' '` 未检查范围，可能索引字体数组前后越界 | 是，传入不可打印或超范围字符时 |
| M-05 | 高 | `Hardware/OLED.c:806-839` | `Image` 未检查空指针，调用时直接读取 `Image[j * Width + i]` | 是，传入空指针时 |
| M-06 | 高 | `Hardware/UltrasonicWave.c:42-75` | 测距和避障缺少超时；`while (T < 15)` 可无限阻塞 | 是，传感器断线、无回波或持续近距离时 |
| M-07 | 中 | `Hardware/Timer.c:3, 35-45` | `Time` 由中断写入、业务代码读取，但未声明 `volatile`；并且 16 位计时值可能回绕 | 高概率，取决于声明位置和编译优化 |
| M-08 | 中 | `System/usart2.c:8-9, 108-118` | ISR 写入 `USART2_RxData/RxFlag`，变量未声明 `volatile`；单字节 flag 模式会覆盖连续输入 | 是，连续数据或高优化级别下 |
| M-09 | 中 | `Hardware/Serial.c:192-200`、`Hardware/OLED.c:854-862` | 格式化 API 参数为可变参数且未限制格式字符串来源，除溢出外还存在错误格式字符串导致的未定义行为 | 取决于调用方 |
| M-10 | 中 | `Hardware/OLED.c:1025-1056` | `uint8_t` 坐标/宽高参与边界循环，输入较大时可能发生索引回绕或异常长循环 | 取决于调用参数 |
| M-11 | 低 | `Hardware/syn6288.c:63-73` | `strlen()` 结果窄化为 `u8 Com_Len`，超过 255 字节会截断发送长度 | 是，但当前协议帧本身已受其他限制 |
| M-12 | 低 | `Hardware/OLED.c:781-791` | 中文字形搜索未找到时仍使用循环结束后的 `pIndex` 读取数据 | 取决于字库表是否包含空哨兵和输入字符 |

## 二、已确认的内存安全问题

### M-01：Serial_Printf 的固定栈缓冲区溢出（已修复）

初始版本在 `Hardware/Serial.c:192-200` 使用 `vsprintf()` 写入 `char String[100]`，存在栈写越界。当前实现已改为 `Serial_Printf(const char *format, ...)`，使用 `vsnprintf(String, sizeof(String), format, arg)`，并在格式化错误时返回 `-1`。

当前安全契约是：返回值小于 0 表示格式化失败；返回值大于等于 100 表示输出被截断；返回值小于 100 表示完整输出长度。调用方可以据此记录截断或错误。输出仍会发送缓冲区中保证 NUL 终止的内容。

### M-02：OLED_Printf 的固定栈缓冲区溢出（已修复）

初始版本在 `Hardware/OLED.c:854-862` 使用 `vsprintf()` 写入 `char String[30]`，任何超过 29 个字符的格式化结果都会越界。当前实现已改为 `OLED_Printf(uint8_t X, uint8_t Y, uint8_t FontSize, const char *format, ...)`，使用 `vsnprintf(String, sizeof(String), format, arg)`。

返回值遵循与 `Serial_Printf()` 相同的约定：负值表示格式化失败，返回值大于等于 30 表示显示文本被截断。OLED 最多显示缓冲区中可容纳的 29 个字符。

## 三、指针、数组和 API 边界风险

### M-03：SYN6288 文本 API 对输入终止条件没有长度保障

`Hardware/syn6288.c:24` 和 `Hardware/syn6288.c:72` 对 `HZdata`/`Info_data` 调用 `strlen()`。虽然代码检查了空指针，但没有接收长度参数，因此只要调用者传入未以 `\0` 结尾的字节数组，`strlen()` 就会一直读取到偶然遇到零字节为止，形成越界读。

`SYN_FrameInfo()` 后续把长度限制到固定帧容量，因此 `memcpy()` 的写入边界目前受到保护，但这不能消除前面的 `strlen()` 越界读取。

**建议修复：** 新增 `SYN_FrameInfoN(u8 music, const u8 *data, uint16_t length)` 和 `YS_SYN_SetN(const u8 *data, uint16_t length)`，由调用方显式传入长度；保留旧 API 时至少在文档中明确要求 NUL 结尾，并对调用点统一使用字符串字面量或已知容量数组。

### M-04：OLED_ShowChar 的字体数组索引缺少范围检查

`Hardware/OLED.c:553-564` 直接使用 `OLED_F8x16[Char - ' ']` 或 `OLED_F6x8[Char - ' ']`。当 `Char < ' '` 或 `Char` 超过字体表支持范围时，减法结果可能为负数或超过数组上界，随后造成越界读。

**建议修复：** 在索引前检查字符是否落在字体表支持范围；不支持的字符显示空格或问号。对于 `char` 的有符号性差异，应先转换为 `uint8_t`，再做明确的区间判断。

### M-05：OLED_ShowImage 只检查坐标，不检查 Image 指针

`Hardware/OLED.c:806-839` 检查了 `X` 和 `Y`，但没有检查 `Image == NULL`，随后在 `OLED_DisplayBuf[...] |= Image[j * Width + i]` 读取图像数据。

此外，`Width` 和 `Height` 没有统一限制到屏幕尺寸，也没有对 `Width == 0`、`Height == 0` 和 `Image` 所指向的实际缓冲区容量进行验证。当前循环会跳过部分超出屏幕的输出，但无法验证输入图像数组本身是否足够大。

**建议修复：** 空指针立即返回；新增带 `image_length` 参数的 API；在进入绘制前检查 `Width/Height` 是否非零，并在调用层保证 `image_length >= ceil(Height/8) * Width`。

### M-10：OLED_DrawRectangle 的无符号边界循环

`Hardware/OLED.c:1025-1056` 使用 `uint8_t i, j`，循环条件却是 `i < X + Width` 和 `j < Y + Height`。当输入坐标和尺寸使上界超过 255 时，`i` 或 `j` 递增到 255 后会回绕到 0，可能造成异常长循环。即使当前 OLED API 通常传入小尺寸，也应在底层函数中拒绝超出屏幕的参数。

**建议修复：** 使用 `uint16_t` 作为循环变量，先计算并裁剪终点；对 `X >= 128`、`Y >= 64`、零宽高和超范围终点统一返回或裁剪。

## 四、未定义行为、并发和资源风险

### M-06：超声波避障存在无界阻塞循环

`Hardware/UltrasonicWave.c:42-46` 每次测距固定 `Delay_ms(100)`，没有等待 Echo 的明确超时状态。`Hardware/UltrasonicWave.c:64-74` 的 `while (T < 15)` 还会持续调用动作和再次测距；当传感器断线、Echo 长时间为低、Timer 没有更新或机器人一直处于近距离环境时，主循环可能永久停留在该函数中。

这不是传统堆泄漏，但会造成任务饥饿，阻止急停、通信处理和其他安全任务执行。对已经迁移到非阻塞状态机的架构而言，这属于严重的实时性和安全风险。

**建议修复：** 将测距拆为 `TRIGGER`、`WAIT_ECHO_HIGH`、`MEASURE`、`TIMEOUT` 状态；所有等待状态使用绝对截止时间；避障动作设置最大尝试次数和总超时，超时后进入安全停止或故障状态。

### M-07：Timer 计时变量的 ISR 共享和回绕

`Hardware/Timer.c:3` 使用 `extern uint16_t Time`，`TIM2_IRQHandler()` 在 `Hardware/Timer.c:35-45` 中递增它，测距代码在 `Hardware/UltrasonicWave.c:46` 读取它。该共享变量没有在声明处体现 `volatile`，若定义处也不是 `volatile`，编译器可能缓存主循环读取结果。

此外，16 位计数器会自然回绕。只要测量逻辑使用差值而不是绝对值，就可以安全处理回绕；当前代码直接把 `Time` 转换为距离，没有区分超时、回绕和无回波。

**建议修复：** 在统一头文件中声明 `extern volatile uint16_t Time`；使用 `uint16_t elapsed = (uint16_t)(now - start)` 计算经过时间；增加最大 Echo 等待时间并将超时作为错误返回。

### M-08：USART2 ISR 共享变量缺少 volatile 且存在丢字节

`System/usart2.c:8-9` 声明 `USART2_RxData` 和 `USART2_RxFlag`，`System/usart2.c:108-118` 在中断中写入它们。当前声明没有 `volatile`。更重要的是，单字节 `RxData + RxFlag` 只能保存最后一个尚未消费的字节；在主循环两次读取之间收到多个字节时，前面的数据会被覆盖。

USART1/USART3 已经使用 `ByteRing`，但 USART2 仍是旧的单字节模式。对于 SYN6288 接收、调试输入或其他连续协议，这会产生数据丢失和状态不同步。

**建议修复：** 为 USART2 增加与 USART1/USART3 相同的环形缓冲区，并将旧 flag/data API 标记为兼容层；所有 ISR 写入、主循环读取的共享状态使用 `volatile` 或通过单生产者/单消费者环形缓冲区封装。

### M-09：可变参数格式字符串造成额外未定义行为面

除了长度溢出，`Serial_Printf()` 和 `OLED_Printf()` 的格式字符串如果与实际参数类型不匹配，例如把整数当作指针或使用错误的长度修饰符，C 的可变参数调用本身就是未定义行为。该风险在裸机代码中很难通过运行时恢复。

**建议修复：** 对日志 API 使用固定格式入口和编译器格式检查属性；在 GCC/Clang 下可使用 `__attribute__((format(printf, 1, 2)))`，Keil 环境则通过封装的强类型接口减少可变参数使用。

## 五、未发现或暂未确认的问题

静态扫描没有发现 `malloc()`、`calloc()`、`realloc()` 或 `free()`，因此当前仓库没有明显的堆生命周期泄漏路径。`ByteRing` 采用固定存储，没有动态分配；`ActionTask`、`AudioTask` 和任务调度器也使用静态数组或静态实例。

已存在的 `tests/host` 主机测试覆盖了 PoseStep 抢占、急停、ByteRing 容量与索引回绕，以及 Timebase 周期回绕。本次修复完成了两个格式化 API 的静态验证，确认业务文件中不再残留对应的 `vsprintf()` 调用。

由于当前环境没有 `arm-none-eabi-gcc`、Keil MDK 或完整 STM32 目标头文件配置，本次不能声称完成了目标芯片级编译、链接、栈使用量测量或 HardFault 运行验证。建议在本地硬件工具链中开启最高级别警告、栈检查和运行时故障记录，再进行一次目标构建。

## 六、建议修复顺序

| 优先级 | 修复内容 | 理由 |
|---|---|---|
| 已完成 | 将两个 `vsprintf()` 改为 `vsnprintf()`，增加返回值、空格式检查和截断契约 | 已消除两处确定性栈缓冲区溢出 |

| P0 | 为超声波等待和避障循环增加截止时间 | 防止阻塞通信和急停路径 |
| P1 | 为 SYN6288 API 增加显式长度参数 | 消除未终止字符串导致的越界读 |
| P1 | 为 OLED 字符、图像和矩形 API 增加统一边界检查 | 消除数组越界和无界循环 |
| P1 | 将 USART2 迁移到 ByteRing，并统一 ISR 共享变量声明 | 消除丢字节和编译优化下的可见性风险 |
| P2 | 将 `Time` 迁移到明确的 volatile 计时接口并增加超时错误码 | 提高测距可靠性和回绕安全性 |
| P2 | 在 GCC/Clang 与 Keil 中启用严格警告和静态分析 | 提前发现格式字符串、隐式转换和未使用返回值问题 |

## References

[1]: https://github.com/RussellCooper-DJZ/manbo-robot-dog/blob/master/Hardware/Serial.c "manbo-robot-dog Hardware/Serial.c"

[2]: https://github.com/RussellCooper-DJZ/manbo-robot-dog/blob/master/Hardware/OLED.c "manbo-robot-dog Hardware/OLED.c"

[3]: https://github.com/RussellCooper-DJZ/manbo-robot-dog/blob/master/Hardware/syn6288.c "manbo-robot-dog Hardware/syn6288.c"

[4]: https://github.com/RussellCooper-DJZ/manbo-robot-dog/blob/master/Hardware/UltrasonicWave.c "manbo-robot-dog Hardware/UltrasonicWave.c"

[5]: https://github.com/RussellCooper-DJZ/manbo-robot-dog/blob/master/Hardware/Timer.c "manbo-robot-dog Hardware/Timer.c"

[6]: https://github.com/RussellCooper-DJZ/manbo-robot-dog/blob/master/System/usart2.c "manbo-robot-dog System/usart2.c"

[7]: https://github.com/RussellCooper-DJZ/manbo-robot-dog/tree/master/tests/host "manbo-robot-dog host tests"
