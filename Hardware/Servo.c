#include "stm32f10x.h"                  // Device header
#include "PWM.h"
// PWM + Servo drive the four SG90s; calibration lives in ServoMath.h
// Servo用于封装舵机的角度设置与读取函数

#include "ServoMath.h"

/* Float API kept for compatibility; gaits use whole degrees anyway. */
static int16_t Servo_Round(float Angle)
{
	return (int16_t)((Angle >= 0.0f) ? (Angle + 0.5f) : (Angle - 0.5f));
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
	PWM_SetCompare1(ServoMath_AngleToCompare(0U, Servo_Round(Angle)));
}

void Servo_SetAngle2(float Angle)
{
	PWM_SetCompare2(ServoMath_AngleToCompare(1U, Servo_Round(Angle)));
}

void Servo_SetAngle3(float Angle)
{
	PWM_SetCompare3(ServoMath_AngleToCompare(2U, Servo_Round(Angle)));
}

void Servo_SetAngle4(float Angle)
{
	PWM_SetCompare4(ServoMath_AngleToCompare(3U, Servo_Round(Angle)));
}

/**
  * 函    数：舵机读取角度
  * 参    数：无
  * 返 回 值：舵机角度，范围：0~180
  */
uint8_t Servo_GetAngle1(void)
{
	return ServoMath_CompareToAngle(TIM_GetCapture1(TIM3));
}

uint8_t Servo_GetAngle2(void)
{
	return ServoMath_CompareToAngle(TIM_GetCapture2(TIM3));
}

uint8_t Servo_GetAngle3(void)
{
	return ServoMath_CompareToAngle(TIM_GetCapture3(TIM3));
}

uint8_t Servo_GetAngle4(void)
{
	return ServoMath_CompareToAngle(TIM_GetCapture4(TIM3));
}
