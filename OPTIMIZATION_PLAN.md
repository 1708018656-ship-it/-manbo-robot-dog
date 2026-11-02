# 曼波机器狗整体优化计划（2026-11-01 ～ 11-04）

基线：上游 `RussellCooper-DJZ/manbo-robot-dog` @ `9a3d315`，工作分支 `optimize-2026-11`。

## 时间安排

| 时段（北京时间） | 重点 |
|---|---|
| 11/01 23:30–24:00 ✅ | 基线审计 + 13 个提交：语音、主循环分发、ASR 协议、非阻塞化、GCC 构建、CI |
| 11/02 08:00–10:00 ✅ | 固件：看门狗、编码统一、超声波测距 bug、电池检测、舵机校准、OLED 非阻塞、死代码清理 |
| 11/03 08:00–10:00 | 硬件：KiCad 原理图 + PCB（STM32 扩展底板）、BOM |
| 11/04 08:00–10:00 | 收尾：ERC/DRC、ASRPro 子项目、CI、文档整合、上传 GitHub |

## 基线审计结论（11/01）

- 主机单元测试：3 组全部通过（state machine 32 / boundary 100 / format contract）。
- 用 clang `--target=thumbv7m-none-eabi` 对 Keil 工程内全部 C 文件做语法检查（本环境装不了 arm-none-eabi-gcc，无法链接出 .hex）。
- 仓库**没有任何原理图 / PCB 文件**，硬件只有文档描述 → 11/03 从零设计。

### 当前固件引脚表（以代码为准，PCB 将按此设计）

| 功能 | 外设 | 引脚 |
|---|---|---|
| 舵机 ×4 | TIM3 CH1–CH4，50 Hz PWM | PA6, PA7, PB0, PB1 |
| 蓝牙 | USART1 | PA9 (TX), PA10 (RX) |
| SYN6288 语音合成 | USART2 | PA2 (TX), PA3 (RX) |
| ASR 语音识别 | USART3 | PB10 (TX), PB11 (RX) |
| 系统时基 | TIM4 1 ms 中断 | — |
| 板载 LED | GPIO | PC13 |
| OLED（软件 I2C） | GPIO | PB8 (SCL), PB9 (SDA) |
| HC-SR04 超声波 | GPIO + TIM2 计时 | PA0 (Trig), PA1 (Echo) |
| 红外 ×4 | GPIO 输入 | PA4, PA5, PA11, PA12 |

### 11/01 发现的硬件级问题（PCB 设计必须解决）

1. **HC-SR04 Echo 是 5 V 输出，直接接 PA1，而 PA0–PA7 不是 5 V 容忍引脚**，长期使用可能损坏 MCU。→ PCB 上加分压（如 1 kΩ/2 kΩ）或换 3.3 V 版本 RCWL-9610。
2. **红外用了 PA11/PA12 = USB D-/D+**。Blue Pill 板载 PA12 有 1.5 kΩ（常见误焊 10 kΩ）上拉到 3.3 V，会干扰红外读数。→ 新底板把 4 路红外移到 PB12–PB15（5 V 容忍、无冲突），固件同步改。
3. **没有电池电压检测**：所有 ADC 引脚都被占用。红外迁走后释放 PA4 → 电池分压采样，低电量自动趴下+语音提醒。
4. 舵机与逻辑共用电源时，4 个 SG90 同时起动的峰值电流（约 2–3 A）会把 3.3 V 拉低导致复位。→ 舵机电源独立 DC-DC + 大电容。

## 已完成

### 11/01
1. **修复：高级动作语音变成拼音字母朗读**（回归 bug）。非阻塞重构时把中文提示语换成了 `"wo ai ni"` 等拼音，SYN6288 会逐个念英文字母。改为 GBK 字节转义（源码保持 ASCII，任何编辑器打开都不会乱码）。
2. **修复：长语音被截断且可能截断半个汉字**。语音帧缓冲只有 50 字节（正文 ≤44 字节），`mode_biaobai()` 的 55 字节提示语会被砍掉尾部，还可能把一个 GBK 双字节汉字截成一半变成杂音。新增 `System/GbkText.h`，按 SYN6288 手册上限 200 字节并保证不拆汉字；阻塞驱动 `syn6288.c` 和非阻塞 `AudioTask.c` 共用。
3. **修复：`AudioTask` 中 `(uint8_t)strlen()` 溢出**，字符串超过 255 字节时长度会回绕。
4. **修复：`syn6288.c` 包含 `"delay.h"`**，实际文件是 `Delay.h`，在 Linux / CI（大小写敏感）上编译失败。
5. 新增主机测试 `tests/host/test_gbk_text.c`（13 项断言），并修正原测试中锁定拼音行为的断言。

### 11/01（续）
6. **修复：主循环命令分发顺序错误，非阻塞状态机被绕过**。`main.c` 先调 `ActionTask_HandleCommand()` 再读串口，于是新收到的运动命令在同一轮直接掉进旧的阻塞式 `mode_xxx()`（`Delay_ms` 步态），下一轮又被 ActionTask 再执行一次：等于每个动作跑两遍，PoseStep 重构对运动命令完全没生效，急停也要等阻塞动作结束。现在先读串口再分发，ActionTask 优先。
7. 保留原版体验：前进/后退/转弯/摇摆/跳舞会一直重复，直到收到新命令（重构后变成只走一步）。
8. 体力/开心值规则从旧 `mode_xxx()` 迁到 `User/PetStats.h`（数值与 Mode.c 一一对应），新增 20 项主机测试；每个命令对应的 OLED 表情保留。
9. 删除没进工程的死代码：`Hardware/Serial.c/.h`（里面有重复的 `USART1_IRQHandler`，一旦加进工程就链接失败）、空的 `Hardware/Mode.c/.h`（会遮蔽 `User/Mode.h`）。
10. 加 `.gitattributes` / `.editorconfig` 统一换行。
11. **ASRPro 协议对齐**：`CMD_STAND='1'`（STM32 上是抬头）改成 `'q'`，`CMD_SIT='3'`（STM32 不处理）改成 `'s'`，握手改成 `'j'`（交替抬手），补全全部命令常量。新增 `docs/protocol.md` 作为唯一权威协议表。
12. **ASRPro**：`delay(6000)` 阻塞改为 `millis()` 截止时间；非运动命令会取消待发送的停止命令；队列空指针保护；删除手工定义的 `__dso_handle`；`commands.h` 补 `<stdint.h>`（原来先包含它会编译失败）。
13. CI：GitHub Actions 每次推送跑主机测试 + `tools/check_firmware_syntax.sh`（clang 对 Keil 工程全部 27 个 C 文件做 Cortex-M3 语法检查，隐式函数声明直接报错）。
14. 换行符统一单独一个提交（`2b6f8bf`，无代码改动），后面的提交 diff 保持干净。
15. **修复：`Time` 在 `UltrasonicWave.c` 定义为普通变量，`Timer.c` 却声明为 `extern volatile`**：两个编译单元限定符不一致，超声波读数可能被编译器缓存。
16. **修复：体力/开心值在 OLED 上只显示 2 位**（体力 350 会显示成 "50"），统一 3 位。`move()` 加 `speed==0` 保护。
17. 新增 `User/BoardConfig.h`：默认保持原版引脚；定义 `MANBO_BOARD_V2` 后切到新 PCB 引脚（红外迁到 PB12–PB15，PA4 用于电池电压采样）。红外驱动改为用宏。

### 11/02
18. **修复：超声波避障基本测不到近处障碍物**。SensorTask 每 10 ms 才轮询一次 Echo 的上升/下降沿，170 cm 以内的回波（< 10 ms）大多整个错过，测到的也偏短。TIM2 中断本来就只在 Echo 为高时累加 100 µs 计数，现在等一个 30 ms 窗口直接读这个计数。新增主机测试按 0.1 ms 精度模拟中断：旧代码 100 cm / 10 cm 都测不到，新代码测 100 cm 得 101 cm、10 cm 触发右转。
19. **独立看门狗 IWDG**（约 1.6 s，调试暂停时冻结，`-DMANBO_NO_WATCHDOG` 可关）。看门狗复位后跳过开机问候，避免复位循环里一直叫。
20. 主循环里不可达的阻塞分发链删掉，换成只处理 n/K/T/Z 的 switch；新增测试保证协议里每个运动字节都由 ActionTask 接管。
21. **全部源码转 UTF-8**（`tools/fix_encoding.py`）：注释转 UTF-8，GBK 字符串改为 `\x` 转义 + 可读注释。逐个对比转换前后的目标文件，**二进制完全一致**。CI 加编码检查。
22. **电池电压检测**（新板 V2，PA4，100k/33k 分压）：8 点滑动平均，低电 6.8 V 语音提醒，6.4 V 趴下并拒绝动作，带 200 mV 回差；13 项测试。原版接线下自动变成空实现。
23. **舵机校准**：`ServoMath.h` 每个舵机可设 `SERVO_TRIM_DEG` 微调角度（装配后站不平时不用拆舵盘），可选软限位；整数运算，脉宽与原来完全一致；15 项测试。
24. **OLED 非阻塞刷新**：换表情不再同步刷整屏 1 KB，改为标记脏页，UI 任务每 10 ms 发一页。
25. 删除死代码：21 个不可达的 `mode_xxx()`、整个 `Movement.c`（步态已全部在 ActionTask 里）、超声波旧包装函数。代码段（不含 libc）从约 35 KB 降到约 22.5 KB。

## 待办（按优先级）

### 固件（11/02）
- [x] 源码编码统一为 UTF-8（11/02）
- [x] 状态播报 K/T/Z 改为非阻塞（11/01）：原来最长 `Delay_s(10)`+阻塞摇摆，期间连急停都不响应；现在合并成一句语音交给 AudioTask，动作交给 ActionTask。也避免了阻塞驱动和 AudioTask 同时往 USART2 写导致语音帧交错
- [x] **修复：体力为 0 时命令被重放**（11/01）。旧代码用不清标志的 `GetRxData()` 判断睡觉命令并阻塞执行，体力恢复后同一字节又被当新命令执行一遍（睡两次、开心值多加 15）。现在字节照常取出，体力 ≤0 时只放行 `2`/`p`，统一走 ActionTask。`mode_nanshou`、`mode_xiaodai` 也改为非阻塞
- [x] 不可达的 `mode_xxx()` 和 `Movement.c` 已清理（11/02）
- [x] OLED 刷新移到 `AppTask_Ui`（11/02）
- [x] GCC 构建（11/01 完成，Keil 工程保留）：`Makefile`、`gcc/STM32F103C8Tx.ld`、`gcc/startup_stm32f103xb.c`（向量表照搬 Keil 启动文件）；`make V2=1` 切新 PCB 引脚。本地用 clang+lld 试链接：只缺 newlib/libgcc 提供的符号，代码段约 35 KB / 64 KB。CI 在 GitHub 上用真正的 arm-none-eabi-gcc 编两种板型并上传 .hex。
- [x] 修复老版 CMSIS `core_cm3.c` 的 `strex*` 内联汇编约束（`"=r"`→`"=&r"`），新版 GCC 会报 "registers may not be the same"
- [ ] 11/02 待确认：CI 上 GCC 实际构建通过（需要先推到 GitHub）
- [x] 超声波测距整数化（11/02）；舵机校准（11/02）
- [ ] 可选：超声波改用 TIM2_CH2 硬件输入捕获（精度从 1.7 cm 提到 mm 级，并去掉 10 kHz 中断）
- [x] 独立看门狗 IWDG（11/02）
- [x] `Edge_detect()` 红外防跌落改为非阻塞：向 ActionTask 提交避险动作（可抢占正在进行的前进），并取消持续行走（11/01 完成）
- [x] MANBO_BOARD_V2：电池 ADC 采样 + 低电量处理（11/02）

### 硬件（11/03）
- [ ] KiCad 8 工程：STM32F103C8T6 最小系统板（Blue Pill）插座式扩展底板
- [ ] 电源：2S 锂电 7.4 V → 舵机 5–6 V 大电流 DC-DC（≥3 A）+ 逻辑 3.3 V LDO 分开供电，舵机电源大电容
- [ ] 电池电压分压采样到 ADC、反接保护、电源开关、保险丝
- [ ] 接口：4×舵机 3P、蓝牙、SYN6288、ASR、OLED(I2C)、HC-SR04（Echo 5V→3.3V 分压）、4×红外、SWD
- [ ] ERC/DRC、BOM（立创商城料号）、Gerber

### 收尾（11/04）
- [ ] GitHub Actions：主机测试 + ARM GCC 构建
- [ ] README 重写：快速开始、接线、协议表、PCB 预览
- [ ] 上传到用户自己的 GitHub 仓库
