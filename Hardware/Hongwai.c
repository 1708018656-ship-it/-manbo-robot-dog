#include "stm32f10x.h"                  // Device header
#include "Movement.h"
#include "Delay.h"
#include "BoardConfig.h"
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
void Edge_detect(void)
{
		t1=hongwai_get_1();
		t2=hongwai_get_2();
		t3=hongwai_get_3();
		t4=hongwai_get_4();
		if(t1==1)
		{
			move_behind();

			t1=0;
			
		}

		if(t2==1)
		{
			move_forward();


			t2=0;
		}


		if(t3==1)
		{
		{
			move_right_hw();
            move_forward();
			move_right_hw();
			move_forward();

			t3=0;
		}
		}
		if(t4==1)
		{
		{
			move_left_hw();
            move_forward();
			move_left_hw();
			move_forward();

			t4=0;
		}
		}
	}
