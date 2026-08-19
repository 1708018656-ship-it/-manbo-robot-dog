# 曼波机器狗（Manbo Robot Dog）

> 基于 STM32F103 的桌面级四足机器狗项目，集成舵机运动控制、OLED 表情显示、蓝牙控制、语音交互、语音播报、红外防跌落和超声波避障功能。

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![MCU](https://img.shields.io/badge/MCU-STM32F103C8T6-03234B.svg)](https://www.st.com/en/microcontrollers-microprocessors/stm32f103c8.html)
[![Voice Module](https://img.shields.io/badge/voice-ESP32%20%2B%20ASR%20Pro-00979D.svg)](ASRPro_Code/README.md)

## 项目简介

曼波机器狗是一个面向嵌入式学习、机器人运动控制和多传感器交互的开源四足机器人项目。项目以 STM32F103C8T6 作为主控制器，通过 PWM 控制舵机完成站立、前进、后退、转向、摇摆、跳舞、打招呼、睡觉等动作；通过 OLED 显示表情和状态信息；通过蓝牙或语音模块接收控制指令；通过 SYN6288 语音合成模块输出提示语音。

当前仓库包含两个相互配合但可以独立编译的工程：

| 工程 | 目录 | 主要职责 | 推荐工具链 |
|---|---|---|---|
| STM32 主控固件 | `Project.uvprojx`、`User/`、`Hardware/`、`System/`、`Library/`、`Start/` | 舵机、传感器、OLED、串口通信、动作和状态管理 | Keil MDK 5.x + STM32F10x 标准外设库 |
| ASR Pro 语音子项目 | `ASRPro_Code/` | ESP32 侧语音命令处理、命令映射和 UART 单字符发送 | PlatformIO + Arduino-ESP32 |

> **实现说明：** 当前 STM32 代码实际以 4 个 PWM 舵机通道进行动作控制；`docs/hardware.md` 中包含更早期的 8 舵机扩展设计。请以源代码、Keil 工程配置和实际电路为准，不要直接将旧版扩展设计当作当前固件的完整接口。

## 功能概览

| 功能类别 | 当前能力 | 主要实现位置 |
|---|---|---|
| 舵机控制 | 4 路 PWM 舵机角度设置、角度读取和边界保护 | `Hardware/Servo.c`、`Hardware/PWM.c` |
| 基础运动 | 站立、前进、后退、左转、右转 | `Hardware/Movement.c` |
| 趣味动作 | 摇摆、跳舞、打招呼、伸懒腰、抬头、握手、睡觉等 | `Hardware/Movement.c`、`User/Mode.c` |
| 蓝牙控制 | USART1 接收单字符控制指令 | `System/usart1.c`、`User/main.c` |
| 语音识别控制 | USART3 接收 ESP32/语音模块转发的单字符指令 | `System/usart3.c`、`User/main.c` |
| 语音合成 | USART2 向 SYN6288 发送语音帧 | `Hardware/syn6288.c`、`System/usart2.c` |
| 超声波避障 | HC-SR04 测距并根据距离执行转向避障 | `Hardware/UltrasonicWave.c` |
| 红外检测 | 四方向红外信号读取和边缘/障碍检测 | `Hardware/Hongwai.c` |
| 表情显示 | OLED 显示站立、移动、睡眠、开心、难受等图像 | `Hardware/OLED.c`、`Hardware/OLED_Data.c` |
| 状态管理 | 体力值、开心值、动作历史和模式标志 | `User/main.c`、`User/Mode.c` |

## 仓库结构

```text
manbo-robot-dog/
├── ASRPro_Code/                 # ESP32 + ASR Pro 语音控制子项目
│   ├── include/                 # ASR 和命令映射头文件
│   ├── src/                     # ASR 主实现、命令映射、Arduino 入口
│   ├── examples/                # 示例代码，不属于默认主程序构建入口
│   ├── docs/                    # ASR 接线、协议和开发说明
│   ├── platformio.ini           # PlatformIO 配置
│   └── README.md                # ASR 子项目说明
├── Hardware/                    # STM32 硬件驱动与机器人执行逻辑
│   ├── PWM.c/h                  # TIM3 四路 PWM
│   ├── Servo.c/h                # 舵机角度封装和角度边界保护
│   ├── Movement.c/h             # 步态与动作序列
│   ├── UltrasonicWave.c/h       # 超声波测距和避障
│   ├── Hongwai.c/h              # 红外传感器和边缘检测
│   ├── OLED.c/h                 # OLED I2C 驱动
│   ├── OLED_Data.c/h            # 字体、图像和字模数据
│   ├── syn6288.c/h              # SYN6288 语音帧封装
│   ├── Serial.c/h               # 另一套串口封装，使用前请确认 Keil 工程是否启用
│   ├── Timer.c/h                # TIM2 测距计时
│   └── LED.c/h                  # LED 指示灯
├── System/                      # 系统延时和 USART 驱动
│   ├── Delay.c/h
│   ├── usart1.c/h               # 蓝牙控制串口
│   ├── usart2.c/h               # SYN6288 语音合成串口
│   └── usart3.c/h               # 语音识别/ESP32 控制串口
├── User/                        # 应用层
│   ├── main.c                   # 初始化、主循环和指令分发
│   ├── Mode.c/h                 # 行为模式、状态值和表情
│   ├── stm32f10x_it.c/h         # 系统中断相关代码
│   └── stm32f10x_conf.h         # STM32 标准外设库配置
├── Library/                     # STM32F10x 标准外设库源码和头文件
├── Start/                       # 启动文件、CMSIS 核心文件和系统时钟
├── docs/                        # 硬件、软件、算法和开发文档
├── Project.uvprojx              # Keil 工程文件
├── Project.uvoptx               # Keil 调试/工程选项
├── LICENSE                      # MIT 许可证
└── README.md                   # 项目总说明
```

## 系统架构

```text
┌─────────────────────────────────────────────────────┐
│                  ESP32 + ASR Pro                    │
│  语音识别 → snid 映射 → 单字符 UART 命令             │
└──────────────────────┬──────────────────────────────┘
                       │ USART3 / 9600 bps
                       ▼
┌─────────────────────────────────────────────────────┐
│                  STM32F103C8T6                       │
│  main.c：初始化、命令优先级、模式分发、状态约束       │
├─────────────────────────────────────────────────────┤
│  Mode.c：行为模式和体力/开心值                        │
│  Movement.c：步态和舵机动作                           │
├─────────────────────────────────────────────────────┤
│  Servo/PWM │ OLED │ USART │ 超声波 │ 红外 │ SYN6288   │
└─────────────────────────────────────────────────────┘
```

主控启动后依次初始化 LED、OLED、USART1/2/3、舵机、红外和超声波模块，然后进入无限主循环。主循环根据体力状态读取蓝牙和语音串口数据，更新控制模式，再执行红外检测、超声波避障和普通动作分发。蓝牙输入具有高于语音输入的优先级；动作函数目前以阻塞式延时为主，因此一个动作执行期间不会像实时操作系统任务那样同时调度所有行为。

## 硬件准备

### 核心器件

| 器件 | 用途 | 备注 |
|---|---|---|
| STM32F103C8T6 | 主控制器 | 72 MHz Cortex-M3，使用 STM32F10x 标准外设库 |
| SG90 舵机 | 四足运动执行器 | 当前固件通过 TIM3 的 4 个通道输出 PWM |
| HC-SR04 | 前方距离测量 | 用于超声波避障 |
| 红外避障/反射传感器 | 边缘和近距离检测 | 用于红外巡航与防跌落逻辑 |
| 0.96 英寸 OLED | 表情和状态显示 | I2C 接口，常见 SSD1306 类模块 |
| HC-05/HC-06 | 蓝牙控制 | USART1，单字符命令 |
| SYN6288 | 中文语音合成 | USART2，使用自定义帧格式 |
| ESP32 + ASR Pro | 离线语音识别 | `ASRPro_Code/` 子项目 |
| 独立舵机电源 | 舵机供电 | 必须与 MCU 电源共地，并预留峰值电流余量 |

### 当前固件使用的接口

以下接口以当前 `Hardware/` 和 `System/` 源码为准。不同 PCB 或扩展版本可能使用不同引脚，首次上电前应通过原理图和万用表再次确认。

| 外设 | 当前代码接口 | 作用 |
|---|---|---|
| 舵机 PWM | PA6、PA7、PB0、PB1 / TIM3 CH1–CH4 | 4 路舵机控制 |
| HC-SR04 Trig | PA0 | 触发超声波 |
| HC-SR04 Echo | PA1 | 读取回波时间 |
| 蓝牙 USART1 | PA9/PA10 | 接收手机或上位机单字符控制 |
| SYN6288 USART2 | PA2/PA3 | 发送语音合成数据帧 |
| 语音/ESP32 USART3 | PB10/PB11 | 接收语音模块转发的单字符命令 |
| OLED I2C | 以 `OLED.c` 的 GPIO 配置为准 | 显示表情和状态 |
| 板载 LED | PC13 | 系统运行或动作指示 |

> 舵机电源不应直接由 STM32 的 3.3 V 稳压输出承担。舵机启动和负载变化会产生较大的瞬时电流，供电不足会造成复位、舵机抖动和串口异常。

## STM32 主控固件

### 开发环境

推荐使用以下环境打开根目录的 `Project.uvprojx`：

| 项目 | 推荐配置 |
|---|---|
| IDE | Keil MDK 5.x |
| 编译器 | ARM Compiler 5 或 ARM Compiler 6 |
| 下载调试器 | ST-Link 或 J-Link |
| 外设库 | STM32F10x Standard Peripheral Library |
| 目标芯片 | STM32F103C8T6 或与工程启动文件匹配的 STM32F1 型号 |

### 编译和烧录

1. 使用 Keil MDK 打开 `Project.uvprojx`。
2. 检查工程中的 Device、Flash Download 和 Debug 设置，确认目标芯片与实际开发板一致。
3. 检查 Include Paths 是否包含 `Start/`、`Library/`、`Hardware/`、`System/` 和 `User/`。
4. 执行 **Rebuild**，确认没有头文件、链接脚本或启动文件错误。
5. 连接 ST-Link 或 J-Link，执行下载并复位运行。
6. 首次测试时建议先断开舵机机械负载，只验证串口、OLED 和单个 PWM 通道，再逐步接入全部执行器。

当前仓库未提供可直接在 Ubuntu 环境中执行的 Keil/ARM GCC 构建脚本；`ASRPro_Code/platformio.ini` 只负责 ESP32 子项目，不能用来编译 STM32 主控工程。

### 板上性能与资源验证

为避免将源码估算误写成实测结论，主控工程提供 `User/PerfProbe.c`，使用 Cortex-M3 的 DWT 周期计数器在协作式调度器中记录通信、安全、传感器、决策、动作、音频和 UI 任务的周期数据。**量产构建默认 `PERF_PROBE_ENABLE=0`，不分配统计数组也不插入采样调用；只有专用测量构建设置 `PERF_PROBE_ENABLE=1` 才启用诊断。** 请参阅 [板上性能测量协议](docs/on_target_performance_measurement.md)，在实际目标板上用 Keil Watch 窗口和同一次构建的 `.map` 文件记录性能基线。

> DWT 采样是工程测量基础设施，不等同于已获得性能结论。旧动作路径、忙等串口和无界测距逻辑仍需完成迁移或单独测量后，才能声明端到端响应指标。

### 主程序运行逻辑

`User/main.c` 的运行逻辑可以概括为：

1. 初始化显示、串口、舵机、红外和超声波模块。
2. 设置初始体力值、开心值和避障标志。
3. OLED 显示初始表情，并通过 SYN6288 播放欢迎语音。
4. 读取 USART1 蓝牙数据和 USART3 语音/ESP32 数据。
5. 优先处理蓝牙输入，然后处理语音输入。
6. 根据 `H/h` 控制红外模式，根据 `x/c` 控制超声波避障模式。
7. 根据动作字符调用 `mode_*()`，由 `Mode.c` 更新状态并调用 `Movement.c` 的动作函数。
8. 当体力耗尽或开心值过低时，进入相应的休息或难受行为。

## 控制指令协议

STM32 主控采用单字符控制协议。蓝牙模块和 ESP32 语音子项目都可以作为发送方，将 ASCII 字符通过 UART 发送给 STM32。

### 通用控制指令

| 字符 | 功能 | STM32 处理位置 |
|---:|---|---|
| `f` | 前进 | `mode_forward()` |
| `b` | 后退 | `mode_behind()` |
| `l` | 左转 | `mode_left()` |
| `r` | 右转 | `mode_right()` |
| `5` | 站立/停止 | `mode_stand()` |
| `w` | 前后摇摆 | `mode_swing_qianhou()` |
| `z` | 左右摇摆 | `mode_swing_zuoyou()` |
| `d` | 跳舞 | `mode_dance()` |
| `q` | 缓慢起身 | `mode_slowstand()` |
| `s` | 坐下/伸展动作 | `mode_strech()` |
| `j` | 交替抬手 | `mode_twohands()` |
| `y` | 伸懒腰 | `mode_lanyao()` |
| `1` | 抬头 | `mode_headup()` |
| `p` | 趴下睡觉 | `mode_sleeppa()` |
| `2` | 卧下睡觉 | `mode_sleepwo()` |
| `n` | 难受/低体力行为 | `mode_nanshou()` |
| `o` | 打招呼 | `mode_hello()` |
| `B` | 表白动作 | `mode_biaobai()` |
| `Y` | 元素周期表动作 | `mode_yuansu()` |
| `X` | 小小训练动作 | `mode_xiaoxun()` |
| `W` | 世界之光动作 | `mode_world()` |
| `U` | 唤醒回应 | `mode_xiaodai()` |
| `K` | 播报开心值 | `mode_happiness()` |
| `T` | 播报体力值 | `mode_stamina()` |
| `Z` | 播报开心值和体力值 | `mode_index()` |

### 传感器模式指令

| 字符 | 功能 |
|---:|---|
| `H` | 开启红外检测/巡航模式 |
| `h` | 关闭红外检测并清零相关传感器状态 |
| `x` | 开启超声波避障 |
| `c` | 关闭超声波避障并恢复默认距离值 |

协议没有消息长度、校验和或序列号。对于蓝牙和语音这种低速控制场景，单字符协议足够简单；如果未来需要连续动作、参数化速度或可靠传输，建议升级为带帧头、长度、命令、参数和校验的协议。

## ESP32 / ASR Pro 语音子项目

### 子项目职责

`ASRPro_Code/` 是一个独立的 PlatformIO 项目，运行在 ESP32 上。它负责创建 ASR 事件队列、根据语音识别 ID 查找命令映射、设置音量或播放音频，并通过 UART 向 STM32 发送单字符命令。

主要文件如下：

| 文件 | 作用 |
|---|---|
| `src/asr.cpp` | ASR 初始化、事件处理、命令发送和 Arduino `setup()`/`loop()` |
| `src/main.cpp` | PlatformIO 主源文件；入口实现统一由 `asr.cpp` 提供 |
| `src/commands.cpp` | 语音识别 ID 到 STM32 单字符命令的映射 |
| `include/asr.h` | ASR 状态、ID、串口参数和公共 API |
| `include/commands.h` | 命令字符宏和映射表接口 |
| `docs/protocol.md` | ESP32 与 STM32 通信协议 |
| `docs/wiring.md` | ESP32、ASR Pro 和 STM32 接线说明 |

### 开发环境和构建

需要安装 VS Code 与 PlatformIO。进入子项目目录后执行：

```bash
cd ASRPro_Code
pio run
pio run --target upload
pio device monitor --baud 115200
```

`platformio.ini` 当前使用 ESP32 Dev Module、Arduino 框架和 C++17 编译选项。ASR Pro SDK 的具体库函数由目标语音模块和 SDK 提供，仓库中的 `asr.cpp` 使用了 `set_state_enter_wakeup()`、`vol_set()`、`play_audio()` 和 `setPinFun()` 等外部接口；如果你的 SDK 版本不同，需要根据 SDK 头文件调整声明和依赖配置。

### 语音命令映射

语音识别返回 `snid` 后，ESP32 在 `g_CommandMap` 中查找对应命令字符。例如，前进映射为 `f`，后退映射为 `b`，左转映射为 `l`，右转映射为 `r`。新增语音命令时，应同步修改：

1. `ASRPro_Code/include/asr.h` 中的识别 ID 或配置常量。
2. `ASRPro_Code/include/commands.h` 中的命令宏。
3. `ASRPro_Code/src/commands.cpp` 中的 `g_CommandMap`。
4. STM32 端 `User/main.c` 的命令分发逻辑。
5. 本 README 和 `ASRPro_Code/docs/protocol.md`。

ESP32 与 STM32 的 UART 连接必须交叉连接：ESP32 TX 接 STM32 RX，ESP32 RX 接 STM32 TX，同时连接公共 GND。当前文档约定通信速率为 9600 bps、8 数据位、无校验、1 停止位。

## 语音合成帧

`Hardware/syn6288.c` 将待播报文本封装为 SYN6288 数据帧，逻辑结构如下：

```text
┌──────┬──────────┬──────┬──────┬──────────────┬──────┐
│ 0xFD │ 长度 2B  │ 0x01 │ 参数 │ 文本数据     │ 校验 │
└──────┴──────────┴──────┴──────┴──────────────┴──────┘
```

当前实现对文本指针和固定帧缓冲区进行边界保护，单帧文本长度受 `SYN_MAX_TEXT_LENGTH` 限制。语音文本的字符编码必须与 SYN6288 模块配置一致；如果出现中文乱码，应优先检查源文件编码、模块编码设置、USART2 波特率和 TX/RX 接线。

## 运动控制与舵机调试

### PWM 与角度映射

`Hardware/Servo.c` 将角度映射为 PWM 比较值：

```text
0°   → 约 500
90°  → 约 1500
180° → 约 2500
```

实际机械零位会受到舵机安装角度、连杆长度和舵机个体误差影响。建议在安装完成后先让所有舵机回到 90°，再逐个调整机械臂和软件动作参数。不要在舵机堵转时长时间保持输出，否则可能导致电机发热、齿轮损坏或电源压降。

### 添加新动作

1. 在 `Hardware/Movement.c` 中定义动作序列，并在 `Hardware/Movement.h` 添加函数声明。
2. 在 `User/Mode.c` 中创建行为模式封装，负责表情、LED、体力和开心值变化。
3. 在 `User/Mode.h` 添加模式函数声明。
4. 在 `User/main.c` 的命令分发链中添加新字符判断。
5. 如果动作通过语音触发，同步修改 `ASRPro_Code/src/commands.cpp` 的映射表。
6. 在低负载、限位安全和急停可用的条件下进行实机测试。

## 调试与故障排查

| 现象 | 优先检查项 |
|---|---|
| 舵机完全不动 | 舵机独立电源、共地、TIM3 时钟、PWM 引脚、舵机插头方向 |
| 舵机抖动或 MCU 复位 | 电源峰值电流、去耦、电源地线、是否同时启动多个舵机 |
| OLED 无显示 | I2C 接线、地址、供电、`OLED_Init()`、显示模块型号 |
| 蓝牙无响应 | USART1 TX/RX 是否交叉、9600 bps、命令字符大小写、接收中断是否启用 |
| 语音无动作 | ESP32 是否成功识别、`snid` 映射、USART3 接线和波特率 |
| 语音有乱码 | SYN6288 供电、USART2 接线、编码格式、文本源文件编码 |
| 超声波距离异常 | Trig/Echo 引脚、传感器供电、Echo 电平、传感器安装方向 |
| 红外模式误触发 | 传感器输出电平逻辑、阈值电位器、传感器安装高度 |
| 编译链接错误 | Keil 工程文件、启动文件、标准外设库路径、目标芯片配置 |

> 由于当前动作控制和传感器测量包含阻塞式延时，长动作期间串口响应可能变慢。若产品需要急停、连续指令或更高实时性，建议将动作和避障逻辑改造成基于定时器的非阻塞状态机。

## 已知限制与后续计划

当前版本更适合作为桌面实验平台和嵌入式学习项目，而不是经过完整安全认证的自主移动机器人。主要限制包括：串口协议较简单、动作控制以阻塞延时为主、传感器异常时需要额外的超时和失效保护、舵机校准依赖具体机械结构，以及 STM32 与 ESP32 工程使用不同工具链。

推荐的后续改进方向包括：

- 使用环形缓冲区替代单字节串口缓存，减少连续指令丢失。
- 将动作、语音播报和超声波测距改造成可打断的非阻塞状态机。
- 为通信协议增加帧头、长度、校验和及急停命令。
- 增加舵机零位、方向和幅度的统一校准参数。
- 为传感器增加超时、异常值滤波和安全降级策略。
- 增加硬件在环测试、串口协议测试和动作回归测试。
- 统一 `Hardware/Serial.c` 与 `System/usart*.c` 的串口抽象，避免维护两套驱动接口。

## 相关文档

| 文档 | 内容 |
|---|---|
| [硬件设计](docs/hardware.md) | 主控、舵机、传感器、通信、电源和装配说明 |
| [软件设计](docs/software.md) | 软件分层、模块设计、主循环、调试与扩展 |
| [代码示例](docs/code_examples.md) | PWM、动作、步态和自定义功能示例 |
| [算法研究](docs/thesis_algorithm_control.md) | 步态、运动控制和避障算法分析 |
| [嵌入式开发说明](docs/thesis_embedded_development.md) | STM32 工程开发和系统设计说明 |
| [ASR Pro 子项目](ASRPro_Code/README.md) | ESP32 语音模块构建和使用说明 |
| [ASR 通信协议](ASRPro_Code/docs/protocol.md) | ESP32 与 STM32 的 UART 协议 |
| [ASR 接线说明](ASRPro_Code/docs/wiring.md) | ASR Pro、ESP32 和 STM32 连接方式 |
| [MIT License](LICENSE) | 项目许可证 |

## 贡献指南

欢迎通过 Issue 或 Pull Request 提交问题、硬件改进、动作算法、文档修订和新传感器适配。提交代码前，请先说明目标硬件版本和工具链，避免将旧版引脚定义、扩展舵机设计或未经验证的性能指标混入当前主工程。

建议每个功能修改保持单一职责，并在提交说明中包含：修改原因、涉及模块、编译环境、实机测试结果和可能的兼容性影响。

## 许可证

本项目采用 [MIT License](LICENSE) 发布。使用、修改和分发本项目时，请保留原许可证和版权声明。

## 致谢

感谢 STM32 标准外设库、Arduino/ESP32 生态和开源硬件社区提供的工具与资料。曼波机器狗的价值不仅在于让四足机器人完成动作，也在于通过真实的机械误差、电源波动、传感器噪声和通信延迟，帮助开发者理解嵌入式系统从代码到实体的完整链路。
