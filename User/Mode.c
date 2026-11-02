#include "stm32f10x.h"                  // Device header
#include "stdlib.h"
#include "LED.h"
#include "Delay.h"
#include "OLED.h"
#include "Servo.h"
#include "PWM.h"
#include "Movement.h"
#include "usart1.h"
#include "usart2.h"
#include "usart3.h"
#include "syn6288.h"
#include "stdio.h"
#include "ActionTask.h"
#include "AudioTask.h"
//***各种状态执行函数***//
//想让动作执行一次后停止，就在末尾加上“	move_mode = '0';”，否则动作持续反复执行，

extern uint8_t move_mode1 ;//用于接收蓝牙串口数据的变量
extern uint8_t move_mode3 ;//用于接收语音识别串口数据的变量
extern uint8_t move_mode ;//决定状态变量
extern uint8_t previous_mode ;//用于保存上一次接收串口数据的变量
extern uint16_t bz_flag;//避障标志位 ON/OFF 默认off
extern uint16_t hw_flag;//红外标志位 ON/OFF 默认off
extern int happiness; //开心指数
extern int stamina;  //体力值
#define STATUS_TEXT_BUFFER_SIZE 128U
static char status_text[STATUS_TEXT_BUFFER_SIZE]={0};  //朗读开心值或体力值的字符串变量

/**************语音合成芯片设置命令*********************/
//选择背景音乐2。(0：无背景音乐  1-15：背景音乐可选)
//m[0~16]:0背景音乐为静音，16背景音乐音量最大
//v[0~16]:0朗读音量为静音，16朗读音量最大
//t[0~5]:0朗读语速最慢，5朗读语速最快

void mode_nanshou(void)//难受（非阻塞）
{
	OLED_ShowImage(0, 0, 128, 64, BMP2); //前进脸
	(void)ActionTask_RequestMotion(ACTION_SLEEP_SIDE);
	(void)AudioTask_RequestText("[v12][m0][t5]\xCE\xD8\xCE\xD8" /* [v12][m0][t5]呜呜 */);
	previous_mode = move_mode;
	move_mode = '0';
}
void OLED_happiness(void)//OLED展示开心值
{
	OLED_ShowChinese(0,0,"\xBF\xAA\xD0\xC4\xD6\xB5\xA3\xAC" /* 开心值， */);
	OLED_ShowNum(64,0,(uint32_t)happiness,3,OLED_8X16); /* max 200/500: always 3 digits */
	OLED_UpdateArea(0,0,128,16);
}
void mode_happiness(void)//展示开心值（非阻塞：一次语音 + 一个动作，不再 Delay_s）
{
	const char *tail = "";
	OLED_ShowImage(0, 8, 128, 48, BMP1); //立正脸
	if (happiness >= 60) {
		OLED_ShowImage(0, 8, 128, 48, BMP9); //开心脸
		tail = "\xA3\xAC\xBA\xCD\xD6\xF7\xC8\xCB\xD4\xDA\xD2\xBB\xC6\xF0\xD5\xE6\xBF\xAA\xD0\xC4" /* ，和主人在一起真开心 */;
		(void)ActionTask_RequestMotion(ACTION_SWING_FORWARD_BACK);
	} else if (happiness <= 10) {
		OLED_ShowImage(0, 8, 128, 48, BMP2);
		tail = "\xA3\xAC\xC5\xE3\xD0\xA1\xB4\xF4\xCD\xE6\xD2\xBB\xBB\xE1\xB0\xC8" /* ，陪小呆玩一会叭 */;
		(void)ActionTask_RequestMotion((rand() % 2) ? ACTION_SLEEP_PRONE : ACTION_SLEEP_SIDE);
	}
	OLED_happiness();
	snprintf(status_text, sizeof(status_text), "[v12][m0][t5]\xB5\xB1\xC7\xB0\xB5\xC4\xBF\xAA\xD0\xC4\xD6\xB5\xCE\xAA%d%s" /* [v12][m0][t5]当前的开心值为%d%s */, happiness, tail);
	(void)AudioTask_RequestText(status_text);
	previous_mode = move_mode;
	move_mode = '0';
}
void OLED_stamina(void)//OLED展示体力值
{
	OLED_ShowChinese(0,0,"\xCC\xE5\xC1\xA6\xD6\xB5\xA3\xAC" /* 体力值， */);
	OLED_ShowNum(64,0,(uint32_t)stamina,3,OLED_8X16); /* max 200/500: always 3 digits */
	OLED_UpdateArea(0,0,128,16);
}
void mode_stamina(void)//展示体力值（非阻塞）
{
	const char *tail = "";
	OLED_ShowImage(0, 8, 128, 48, BMP1); //立正脸
	if (stamina >= 60) {
		OLED_ShowImage(0, 8, 128, 48, BMP9); //开心脸
		tail = "\xA3\xAC\xD0\xA1\xB4\xF4\xBE\xAB\xC1\xA6\xC2\xFA\xC2\xFA\xA3\xAC\xBA\xCD\xCE\xD2\xD2\xBB\xC6\xF0\xCD\xE6\xB0\xC8" /* ，小呆精力满满，和我一起玩叭 */;
		(void)ActionTask_RequestMotion(ACTION_SWING_FORWARD_BACK);
	} else if (stamina <= 10) {
		OLED_ShowImage(0, 8, 128, 48, BMP2);
		tail = "\xA3\xAC\xC8\xC3\xD0\xA1\xB4\xF4\xD0\xDD\xCF\xA2\xD2\xBB\xCF\xC2\xB0\xC8" /* ，让小呆休息一下叭 */;
		(void)ActionTask_RequestMotion((rand() % 2) ? ACTION_SLEEP_PRONE : ACTION_SLEEP_SIDE);
	}
	OLED_stamina();
	snprintf(status_text, sizeof(status_text), "[v12][m0][t5]\xB5\xB1\xC7\xB0\xB5\xC4\xCC\xE5\xC1\xA6\xD6\xB5\xCE\xAA%d%s" /* [v12][m0][t5]当前的体力值为%d%s */, stamina, tail);
	(void)AudioTask_RequestText(status_text);
	previous_mode = move_mode;
	move_mode = '0';
}
void mode_index(void)//展示开心值和体力值
{
	OLED_Clear();
	OLED_ShowChinese(0,0,"\xBF\xAA\xD0\xC4\xD6\xB5\xA3\xAC" /* 开心值， */);
	OLED_ShowNum(64,0,(uint32_t)happiness,3,OLED_8X16); /* max 200/500: always 3 digits */
	OLED_ShowChinese(0,16,"\xCC\xE5\xC1\xA6\xD6\xB5\xA3\xAC" /* 体力值， */);
	OLED_ShowNum(64,16,(uint32_t)stamina,3,OLED_8X16); /* max 200/500: always 3 digits */
	OLED_UpdateArea(0,0,128,32);
	snprintf(status_text, sizeof(status_text), "[v12][m0][t5]\xB5\xB1\xC7\xB0\xB5\xC4\xBF\xAA\xD0\xC4\xD6\xB5\xCE\xAA%d\xA3\xAC\xCC\xE5\xC1\xA6\xD6\xB5\xCE\xAA%d" /* [v12][m0][t5]当前的开心值为%d，体力值为%d */, happiness, stamina);
	(void)AudioTask_RequestText(status_text); /* 非阻塞，原来 Delay_s(10) */
	previous_mode = move_mode;
	move_mode = '0';
}

