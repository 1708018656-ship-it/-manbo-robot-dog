#include "stm32f10x.h"                  // Device header
#include "PWM.h"
// PWM、Servo、Movement三个文件共同为驱动舵机服务
// Servo用于封装舵机的角度设置与读取函数

#define SERVO_MIN_ANGLE       0.0f
#define SERVO_MAX_ANGLE       180.0f
#define SERVO_MIN_COMPARE     500U
#define SERVO_MAX_COMPARE     2500U
#define SERVO_COMPARE_RANGE   (SERVO_MAX_COMPARE - SERVO_MIN_COMPARE)

static uint16_t Servo_AngleToCompare(float Angle)
{
	if (Angle <= SERVO_MIN_ANGLE)
	{
		return SERVO_MIN_COMPARE;
	}
	if (Angle >= SERVO_MAX_ANGLE)
	{
		return SERVO_MAX_COMPARE;
	}

	return (uint16_t)(Angle * SERVO_COMPARE_RANGE / SERVO_MAX_ANGLE + SERVO_MIN_COMPARE);
}

static uint8_t Servo_CompareToAngle(uint16_t Compare)
{
	if (Compare <= SERVO_MIN_COMPARE)
	{
		return (uint8_t)SERVO_MIN_ANGLE;
	}
	if (Compare >= SERVO_MAX_COMPARE)
	{
		return (uint8_t)SERVO_MAX_ANGLE;
	}

	return (uint8_t)(((uint32_t)(Compare - SERVO_MIN_COMPARE) * 180U) / SERVO_COMPARE_RANGE);
}

/**
  * 函    数：舵机初始化
  * 参    数：无
  * 返 回 值：无
  */
void Servo_Init(void)
{
	PWM_Init();
}

/**
  * 函    数：舵机设置角度
  * 参    数：Angle 要设置的舵机角度，范围：0~180
  * 返 回 值：无
  */
void Servo_SetAngle1(float Angle)
{
	PWM_SetCompare1(Servo_AngleToCompare(Angle));
}

void Servo_SetAngle2(float Angle)
{
	PWM_SetCompare2(Servo_AngleToCompare(Angle));
}

void Servo_SetAngle3(float Angle)
{
	PWM_SetCompare3(Servo_AngleToCompare(Angle));
}

void Servo_SetAngle4(float Angle)
{
	PWM_SetCompare4(Servo_AngleToCompare(Angle));
}

/**
  * 函    数：舵机读取角度
  * 参    数：无
  * 返 回 值：舵机角度，范围：0~180
  */
uint8_t Servo_GetAngle1(void)
{
	return Servo_CompareToAngle(TIM_GetCapture1(TIM3));
}

uint8_t Servo_GetAngle2(void)
{
	return Servo_CompareToAngle(TIM_GetCapture2(TIM3));
}

uint8_t Servo_GetAngle3(void)
{
	return Servo_CompareToAngle(TIM_GetCapture3(TIM3));
}

uint8_t Servo_GetAngle4(void)
{
	return Servo_CompareToAngle(TIM_GetCapture4(TIM3));
}
