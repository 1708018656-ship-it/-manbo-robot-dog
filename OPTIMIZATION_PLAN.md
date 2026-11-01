# 曼波机器狗整体优化计划（2026-11-01 ～ 11-04）

基线：上游 `RussellCooper-DJZ/manbo-robot-dog` @ `9a3d315`，工作分支 `optimize-2026-11`。

## 时间安排

| 时段（北京时间） | 重点 |
|---|---|
| 11/01 23:30–24:00 | 基线审计、构建检查、修复语音合成编码问题 |
| 11/02 08:00–10:00 | 固件：命令协议统一、编码统一、剩余 bug、ARM 构建脚本 |
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
| OLED / 超声波 / 红外 | 待 11/02 核对 | — |

## 已完成

### 11/01
1. **修复：高级动作语音变成拼音字母朗读**（回归 bug）。非阻塞重构时把中文提示语换成了 `"wo ai ni"` 等拼音，SYN6288 会逐个念英文字母。改为 GBK 字节转义（源码保持 ASCII，任何编辑器打开都不会乱码）。
2. **修复：长语音被截断且可能截断半个汉字**。语音帧缓冲只有 50 字节（正文 ≤44 字节），`mode_biaobai()` 的 55 字节提示语会被砍掉尾部，还可能把一个 GBK 双字节汉字截成一半变成杂音。新增 `System/GbkText.h`，按 SYN6288 手册上限 200 字节并保证不拆汉字；阻塞驱动 `syn6288.c` 和非阻塞 `AudioTask.c` 共用。
3. **修复：`AudioTask` 中 `(uint8_t)strlen()` 溢出**，字符串超过 255 字节时长度会回绕。
4. **修复：`syn6288.c` 包含 `"delay.h"`**，实际文件是 `Delay.h`，在 Linux / CI（大小写敏感）上编译失败。
5. 新增主机测试 `tests/host/test_gbk_text.c`（13 项断言），并修正原测试中锁定拼音行为的断言。

## 待办（按优先级）

### 固件（11/02）
- [ ] ASRPro `commands.h` 中 `CMD_STAND='1'`、`CMD_SIT='3'` 等与 STM32 实际协议不一致 → 生成一份共享协议表，两端对齐
- [ ] ASRPro 手工定义 `__dso_handle`、6 s 阻塞发送 → 改为非阻塞
- [ ] 源码编码混用（UTF-8 / GBK / CRLF+LF 混排）→ 统一：注释 UTF-8，GBK 文本一律用转义；加 `.gitattributes` / `.editorconfig`
- [ ] `Hardware/Serial.c`、空的 `Hardware/Mode.c/.h` 未进工程 → 删除或归档，避免误用
- [ ] 加 GCC + Makefile/CMake 构建（Keil 工程保留），链接脚本 `STM32F103C8Tx_FLASH.ld`
- [ ] 超声波测距整数化、舵机角度斜坡限速、低电压检测（需 ADC 引脚，与 PCB 联动）
- [ ] 加独立看门狗 IWDG

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
