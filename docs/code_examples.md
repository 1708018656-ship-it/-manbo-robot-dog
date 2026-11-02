# 代码示例文档

> ⚠️ **早期资料，与当前代码不一致**：本文描述的是早期的扩展设想（如 8/12 个舵机或 STM32F407），当前固件是 STM32F103C8T6 + 4 个舵机。以 [README](../README.md)、[docs/protocol.md](protocol.md) 和源码为准。

本文档提供曼波机器狗项目的核心代码示例,所有代码均来自实际项目。

## 📋 目录

1. [PWM初始化](#pwm初始化)
2. [舵机控制](#舵机控制)
3. [运动控制](#运动控制)
4. [平滑运动](#平滑运动)

---

## PWM初始化

### 完整的PWM初始化代码

```c
#include "stm32f10x.h"

/**
  * 函    数:PWM初始化
  * 参    数:无
  * 返 回 值:无
  * 说    明:配置TIM3的4个通道输出PWM信号,用于控制4个舵机
  */
void PWM_Init(void)
{
    /* 开启时钟 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);    // 开启TIM3的时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);   // 开启GPIOA的时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);   // 开启GPIOB的时钟
    
    /* GPIO初始化 */
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;         // 复用推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7;    // PA6/PA7
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);                  // 将PA6/7引脚初始化为复用推挽输出
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1;    // PB0/PB1
    GPIO_Init(GPIOB, &GPIO_InitStructure);                  // 将PB0/1引脚初始化为复用推挽输出
    
    /* 配置时钟源 */
    TIM_InternalClockConfig(TIM3);                          // 选择TIM3为内部时钟
    
    /* 时基单元初始化 */
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;      // 时钟分频,选择不分频
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;  // 计数器模式,选择向上计数
    TIM_TimeBaseInitStructure.TIM_Period = 20000 - 1;                // 计数周期,ARR = 20000-1 (20ms)
    TIM_TimeBaseInitStructure.TIM_Prescaler = 72 - 1;                // 预分频器,PSC = 72-1 (1MHz)
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;             // 重复计数器
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStructure);
    
    /* 输出比较初始化 */
    TIM_OCInitTypeDef TIM_OCInitStructure;
    TIM_OCStructInit(&TIM_OCInitStructure);                          // 结构体初始化
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;                // 输出比较模式,选择PWM模式1
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;        // 输出极性,选择为高
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;    // 输出使能
    TIM_OCInitStructure.TIM_Pulse = 0;                               // 初始的CCR值
    
    // 初始化4个通道
    TIM_OC1Init(TIM3, &TIM_OCInitStructure);  // CH1 - PA6 - 舵机1(左前腿)
    TIM_OC2Init(TIM3, &TIM_OCInitStructure);  // CH2 - PA7 - 舵机2(左后腿)
    TIM_OC3Init(TIM3, &TIM_OCInitStructure);  // CH3 - PB0 - 舵机3(右前腿)
    TIM_OC4Init(TIM3, &TIM_OCInitStructure);  // CH4 - PB1 - 舵机4(右后腿)
    
    /* TIM使能 */
    TIM_Cmd(TIM3, ENABLE);  // 使能TIM3,定时器开始运行
}

/**
  * 函    数:PWM设置CCR
  * 参    数:Compare 要写入的CCR的值,范围:500~2500
  * 返 回 值:无
  * 说    明:CCR和ARR共同决定占空比
  *          占空比Duty = CCR / (ARR + 1)
  *          500对应0.5ms(0°), 1500对应1.5ms(90°), 2500对应2.5ms(180°)
  */
void PWM_SetCompare1(uint16_t Compare)
{
    TIM_SetCompare1(TIM3, Compare);
}

void PWM_SetCompare2(uint16_t Compare)
{
    TIM_SetCompare2(TIM3, Compare);
}

void PWM_SetCompare3(uint16_t Compare)
{
    TIM_SetCompare3(TIM3, Compare);
}

void PWM_SetCompare4(uint16_t Compare)
{
    TIM_SetCompare4(TIM3, Compare);
}
```

**PWM参数说明**:
- **定时器频率**: 72MHz / 72 = 1MHz (计数频率)
- **PWM周期**: 20000 / 1MHz = 20ms (50Hz)
- **脉宽范围**: 0.5ms ~ 2.5ms
- **角度对应**:
  - 0° → CCR = 500 (0.5ms)
  - 90° → CCR = 1500 (1.5ms)
  - 180° → CCR = 2500 (2.5ms)

---

## 舵机控制

### 舵机初始化

```c
#include "stm32f10x.h"
#include "PWM.h"

/**
  * 函    数:舵机初始化
  * 参    数:无
  * 返 回 值:无
  */
void Servo_Init(void)
{
    PWM_Init();  // 初始化舵机的底层PWM
}
```

### 舵机角度设置

```c
/**
  * 函    数:舵机设置角度
  * 参    数:Angle 要设置的舵机角度,范围:0~180
  * 返 回 值:无
  * 说    明:角度到PWM脉宽的转换公式
  *          PWM = Angle / 180 * 2000 + 500
  */
void Servo_SetAngle1(float Angle)
{
    PWM_SetCompare1(Angle / 180 * 2000 + 500);
}

void Servo_SetAngle2(float Angle)
{
    PWM_SetCompare2(Angle / 180 * 2000 + 500);
}

void Servo_SetAngle3(float Angle)
{
    PWM_SetCompare3(Angle / 180 * 2000 + 500);
}

void Servo_SetAngle4(float Angle)
{
    PWM_SetCompare4(Angle / 180 * 2000 + 500);
}
```

### 舵机角度读取

```c
/**
  * 函    数:舵机读取角度
  * 参    数:无
  * 返 回 值:舵机角度,范围:0~180
  * 说    明:从CCR值反算角度
  */
uint8_t Servo_GetAngle1(void)
{
    return (TIM_GetCapture1(TIM3) - 500) * 180 / 2000;
}

uint8_t Servo_GetAngle2(void)
{
    return (TIM_GetCapture2(TIM3) - 500) * 180 / 2000;
}

uint8_t Servo_GetAngle3(void)
{
    return (TIM_GetCapture3(TIM3) - 500) * 180 / 2000;
}

uint8_t Servo_GetAngle4(void)
{
    return (TIM_GetCapture4(TIM3) - 500) * 180 / 2000;
}
```

---

## 运动控制

### 全局变量

```c
#include "stm32f10x.h"
#include "Servo.h"
#include "PWM.h"
#include "Delay.h"
#include "stdlib.h"

int i, j;
int movedelay = 150;  // 改变这里来改变动作间隔(ms)
uint8_t angle_1, angle_2, angle_3, angle_4;
```

### 站立姿态

```c
/**
  * 函    数:站立
  * 参    数:无
  * 返 回 值:无
  * 说    明:所有舵机回到90°中位
  */
void move_stand(void)
{
    Servo_SetAngle1(90);
    Servo_SetAngle2(90);
    Servo_SetAngle3(90);
    Servo_SetAngle4(90);
    Delay_ms(500);
}
```

### 前进步态

```c
/**
  * 函    数:前进
  * 参    数:无
  * 返 回 值:无
  * 说    明:采用对角步态,左前右后为一组,右前左后为一组
  *          舵机1:左前腿, 舵机2:左后腿, 舵机3:右前腿, 舵机4:右后腿
  */
void move_forward(void)
{
    // 第一组对角腿抬起(左前+右后)
    Servo_SetAngle1(135);  // 左前腿向内摆
    Servo_SetAngle4(45);   // 右后腿向外摆
    Delay_ms(movedelay);
    
    // 第二组对角腿抬起(右前+左后)
    Servo_SetAngle2(45);   // 左后腿向外摆
    Servo_SetAngle3(135);  // 右前腿向内摆
    Delay_ms(movedelay);
    
    // 第一组对角腿落下
    Servo_SetAngle1(90);
    Servo_SetAngle4(90);
    Delay_ms(movedelay);
    
    // 第二组对角腿落下
    Servo_SetAngle2(90);
    Servo_SetAngle3(90);
    Delay_ms(movedelay);
    
    // 第二组对角腿抬起(左后+右前)
    Servo_SetAngle2(135);  // 左后腿向内摆
    Servo_SetAngle3(45);   // 右前腿向外摆
    Delay_ms(movedelay);
    
    // 第一组对角腿抬起(左前+右后)
    Servo_SetAngle1(45);   // 左前腿向外摆
    Servo_SetAngle4(135);  // 右后腿向内摆
    Delay_ms(movedelay);
    
    // 第二组对角腿落下
    Servo_SetAngle2(90);
    Servo_SetAngle3(90);
    Delay_ms(movedelay);
    
    // 第一组对角腿落下
    Servo_SetAngle1(90);
    Servo_SetAngle4(90);
    Delay_ms(movedelay);
}
```

### 后退步态

```c
/**
  * 函    数:后退
  * 参    数:无
  * 返 回 值:无
  * 说    明:与前进相反的步态序列
  */
void move_behind(void)
{
    Servo_SetAngle1(45);
    Servo_SetAngle4(135);
    Delay_ms(movedelay);
    
    Servo_SetAngle2(135);
    Servo_SetAngle3(45);
    Delay_ms(movedelay);
    
    Servo_SetAngle1(90);
    Servo_SetAngle4(90);
    Delay_ms(movedelay);
    
    Servo_SetAngle2(90);
    Servo_SetAngle3(90);
    Delay_ms(movedelay);
    
    Servo_SetAngle2(45);
    Servo_SetAngle3(135);
    Delay_ms(movedelay);
    
    Servo_SetAngle1(135);
    Servo_SetAngle4(45);
    Delay_ms(movedelay);
    
    Servo_SetAngle2(90);
    Servo_SetAngle3(90);
    Delay_ms(movedelay);
    
    Servo_SetAngle1(90);
    Servo_SetAngle4(90);
    Delay_ms(movedelay);
}
```

### 右转

```c
/**
  * 函    数:右转
  * 参    数:无
  * 返 回 值:无
  * 说    明:左右两侧同时向同一方向摆动
  */
void move_right(void)
{
    Servo_SetAngle1(45);
    Servo_SetAngle4(45);
    Delay_ms(movedelay);
    
    Servo_SetAngle2(135);
    Servo_SetAngle3(135);
    Delay_ms(movedelay);
    
    Servo_SetAngle1(90);
    Servo_SetAngle4(90);
    Delay_ms(movedelay);
    
    Servo_SetAngle2(90);
    Servo_SetAngle3(90);
    Delay_ms(movedelay);
}
```

### 左转

```c
/**
  * 函    数:左转
  * 参    数:无
  * 返 回 值:无
  */
void move_left(void)
{
    Servo_SetAngle2(135);
    Servo_SetAngle3(135);
    Delay_ms(movedelay);
    
    Servo_SetAngle1(45);
    Servo_SetAngle4(45);
    Delay_ms(movedelay);
    
    Servo_SetAngle2(90);
    Servo_SetAngle3(90);
    Delay_ms(movedelay);
    
    Servo_SetAngle1(90);
    Servo_SetAngle4(90);
    Delay_ms(movedelay);
}
```

---

## 平滑运动

### 通用平滑运动函数

```c
/**
  * 函    数:平滑运动到指定角度
  * 参    数:set1~set4 目标角度, speed 运动速度(度/秒)
  * 返 回 值:无
  * 说    明:逐度递增/递减,实现平滑过渡
  */
void move(uint16_t set1, uint16_t set2, uint16_t set3, uint16_t set4, uint16_t speed)
{
    // 读取当前角度
    angle_1 = Servo_GetAngle1();
    angle_2 = Servo_GetAngle2();
    angle_3 = Servo_GetAngle3();
    angle_4 = Servo_GetAngle4();
    
    // 循环直到所有舵机到达目标角度
    while (angle_1 != set1 || angle_3 != set3 || angle_2 != set2 || angle_4 != set4) {
        // 舵机1
        if (angle_1 > set1) {
            --angle_1;
            Servo_SetAngle1(angle_1);
        } else if (angle_1 < set1) {
            ++angle_1;
            Servo_SetAngle1(angle_1);
        }
        
        // 舵机3
        if (angle_3 > set3) {
            --angle_3;
            Servo_SetAngle3(angle_3);
        } else if (angle_3 < set3) {
            ++angle_3;
            Servo_SetAngle3(angle_3);
        }
        
        // 舵机2
        if (angle_2 > set2) {
            --angle_2;
            Servo_SetAngle2(angle_2);
        } else if (angle_2 < set2) {
            ++angle_2;
            Servo_SetAngle2(angle_2);
        }
        
        // 舵机4
        if (angle_4 > set4) {
            --angle_4;
            Servo_SetAngle4(angle_4);
        } else if (angle_4 < set4) {
            ++angle_4;
            Servo_SetAngle4(angle_4);
        }
        
        // 延时控制速度: 1000ms/speed = 每度的延时
        Delay_ms(1000/speed);
    }
}
```

**使用示例**:
```c
// 以60度/秒的速度平滑移动到指定角度
move(45, 135, 135, 45, 60);
```

### 缓慢起身

```c
/**
  * 函    数:缓慢起身
  * 参    数:previous_mode 之前的模式
  * 返 回 值:无
  * 说    明:从任意姿态平滑回到站立姿态(90°)
  */
void move_slow_stand(uint8_t previous_mode)
{
    if (previous_mode == '0')
        return;
    
    // 读取当前角度
    angle_1 = Servo_GetAngle1();
    angle_2 = Servo_GetAngle2();
    angle_3 = Servo_GetAngle3();
    angle_4 = Servo_GetAngle4();
    
    // 逐度调整到90°
    while (angle_1 != 90 || angle_3 != 90 || angle_2 != 90 || angle_4 != 90) {
        if (angle_1 > 90) {
            --angle_1;
            Servo_SetAngle1(angle_1);
        } else if (angle_1 < 90) {
            ++angle_1;
            Servo_SetAngle1(angle_1);
        }
        
        if (angle_3 > 90) {
            --angle_3;
            Servo_SetAngle3(angle_3);
        } else if (angle_3 < 90) {
            ++angle_3;
            Servo_SetAngle3(angle_3);
        }
        
        if (angle_2 > 90) {
            --angle_2;
            Servo_SetAngle2(angle_2);
        } else if (angle_2 < 90) {
            ++angle_2;
            Servo_SetAngle2(angle_2);
        }
        
        if (angle_4 > 90) {
            --angle_4;
            Servo_SetAngle4(angle_4);
        } else if (angle_4 < 90) {
            ++angle_4;
            Servo_SetAngle4(angle_4);
        }
        
        Delay_ms(10);  // 每度延时10ms
    }
}
```

---

## 趣味动作

### 打招呼

```c
/**
  * 函    数:打招呼
  * 参    数:无
  * 返 回 值:无
  * 说    明:抬起右前腿并摇动
  */
void move_hello(void)
{
    // 身体向左倾斜
    for(i=0; i<20; i++)
    {
        Servo_SetAngle1(90-i);
        Servo_SetAngle2(90+i);
        Servo_SetAngle4(90-i);
        Delay_ms(4);
    }
    
    // 后腿蹲下
    for(i=0; i<60; i++)
    {
        Servo_SetAngle2(110+i);
        Servo_SetAngle4(70-i);
        Delay_ms(4);
    }
    
    // 抬起右前腿
    for(i=0; i<60; i++)
    {
        Servo_SetAngle1(70+i);
        Delay_ms(4);
    }
    Delay_ms(50);
    
    // 摇动右前腿两次
    Servo_SetAngle1(180);
    Delay_ms(500);
    Servo_SetAngle1(130);
    Delay_ms(500);
    Servo_SetAngle1(180);
    Delay_ms(500);
    Servo_SetAngle1(130);
    Delay_ms(500);
    Servo_SetAngle1(70);
    Delay_ms(500);
}
```

### 趴下睡觉

```c
/**
  * 函    数:趴下睡觉
  * 参    数:无
  * 返 回 值:无
  * 说    明:前腿向外,后腿向内,趴在地上
  */
void move_sleep_p(void)
{
    // 前腿缓慢向外摆
    for(i=0; i<75; i++)
    {
        Servo_SetAngle1(90+i);  // 左前腿向外
        Servo_SetAngle3(90-i);  // 右前腿向外
        Delay_ms(10);
    }
    
    // 后腿缓慢向内摆
    for(i=0; i<75; i++)
    {
        Servo_SetAngle4(90+i);  // 右后腿向内
        Servo_SetAngle2(90-i);  // 左后腿向内
        Delay_ms(10);
    }
}
```

### 伸懒腰

```c
/**
  * 函    数:伸懒腰
  * 参    数:无
  * 返 回 值:无
  * 说    明:前腿向外伸展,后腿配合
  */
void lan_yao(void)
{
    // 向外伸展
    for(i=0; i<75; i++)
    {
        Servo_SetAngle1(90+i);
        Servo_SetAngle3(90-i);
        Servo_SetAngle2(90+i/2);
        Servo_SetAngle4(90-i/2);
        Delay_ms(5);
    }
    Delay_ms(movedelay*50);  // 保持伸展姿态
    
    // 收回
    for(i=0; i<75; i++)
    {
        Servo_SetAngle1(165-i);
        Servo_SetAngle3(15+i);
        Servo_SetAngle2(127-i/2);
        Servo_SetAngle4(53+i/2);
        Delay_ms(5);
    }
    Delay_ms(movedelay);
}
```

---

## 使用说明

### 调整运动速度

修改全局变量 `movedelay` 来调整运动速度:

```c
int movedelay = 150;  // 默认150ms

// 增大延时 → 运动变慢但更稳定
int movedelay = 200;

// 减小延时 → 运动变快但可能不稳
int movedelay = 100;
```

### 自定义动作

参考现有动作,创建自己的动作序列:

```c
void my_custom_action(void)
{
    // 步骤1: 设置舵机角度
    Servo_SetAngle1(45);
    Servo_SetAngle2(135);
    Servo_SetAngle3(135);
    Servo_SetAngle4(45);
    Delay_ms(movedelay);
    
    // 步骤2: 回到中位
    Servo_SetAngle1(90);
    Servo_SetAngle2(90);
    Servo_SetAngle3(90);
    Servo_SetAngle4(90);
    Delay_ms(movedelay);
}
```

### 角度范围建议

- **安全范围**: 45° ~ 135° (避免舵机堵转)
- **中位**: 90° (站立姿态)
- **最大摆幅**: ±45° (相对于中位)

---

**文档版本**: v1.0  
**更新日期**: 2025-10-24  
**作者**: Manbo Robot Dog Team

