#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "BoardConfig.h"
#include "ActionTask.h"
//***红外悬崖检测***//
extern uint8_t t1 ; //存储红外的信号 前
extern uint8_t t2 ; //存储红外的信号 后
extern uint8_t t3 ; //存储红外的信号 左
extern uint8_t t4 ; //存储红外的信号 右
void hongwai_init(void) //红外初始化，下拉输入（常态为 0，检测到悬崖为 1）
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(IR_GPIO_CLK, ENABLE);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

	GPIO_InitStructure.GPIO_Pin = IR_FRONT_PIN;
	GPIO_Init(IR_FRONT_PORT, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = IR_BACK_PIN;
	GPIO_Init(IR_BACK_PORT, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = IR_LEFT_PIN;
	GPIO_Init(IR_LEFT_PORT, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = IR_RIGHT_PIN;
	GPIO_Init(IR_RIGHT_PORT, &GPIO_InitStructure);
}

uint8_t hongwai_get_1(void) //前
{
	return GPIO_ReadInputDataBit(IR_FRONT_PORT, IR_FRONT_PIN);
}
uint8_t hongwai_get_2(void) //后
{
	return GPIO_ReadInputDataBit(IR_BACK_PORT, IR_BACK_PIN);
}
uint8_t hongwai_get_3(void) //左
{
	return GPIO_ReadInputDataBit(IR_LEFT_PORT, IR_LEFT_PIN);
}
uint8_t hongwai_get_4(void) //右
{
	return GPIO_ReadInputDataBit(IR_RIGHT_PORT, IR_RIGHT_PIN);
}
/*
 * 红外防跌落（非阻塞）。
 * 原实现直接调用 move_behind()/move_forward() 等 Delay_ms 阻塞动作，
 * 期间串口急停和其它任务都无法响应。现在只向 ActionTask 提交一个避险动作
 * （ActionTask 支持抢占，正在前进时会立刻被替换），由调度器按步推进。
 * 返回 1 表示本轮触发了避险，调用方应取消持续行走。
 */
uint8_t Edge_detect(void)
{
	t1 = hongwai_get_1();
	t2 = hongwai_get_2();
	t3 = hongwai_get_3();
	t4 = hongwai_get_4();

	if (t1 == 1U)        /* 前方悬崖 -> 后退 */
	{
		(void)ActionTask_RequestMotion(ACTION_BACKWARD);
	}
	else if (t2 == 1U)   /* 后方悬崖 -> 前进 */
	{
		(void)ActionTask_RequestMotion(ACTION_FORWARD);
	}
	else if (t3 == 1U)   /* 左侧悬崖 -> 右转 */
	{
		(void)ActionTask_RequestMotion(ACTION_TURN_RIGHT);
	}
	else if (t4 == 1U)   /* 右侧悬崖 -> 左转 */
	{
		(void)ActionTask_RequestMotion(ACTION_TURN_LEFT);
	}
	else
	{
		return 0U;
	}
	t1 = 0U; t2 = 0U; t3 = 0U; t4 = 0U;
	return 1U;
}
