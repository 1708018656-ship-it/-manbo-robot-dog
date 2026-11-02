#include "stm32f10x.h"                  // Device header
#include "stdlib.h"
#include <stdio.h>
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
#include "Hongwai.h"
#include "UltrasonicWave.h"
#include "SensorTask.h"
#include "Mode.h"
#include "Timebase.h"
#include "AppTask.h"
#include "CommTask.h"
#include "SafetyTask.h"
#include "ActionTask.h"
#include "AudioTask.h"
#include "PetStats.h"
#include "Watchdog.h"

/****************************************************
*
*³ÌĞòÃû³Æ£ºµç×Ó×ÀÃæ³èÎï¡°Ğ¡´ô¡±
*µ±Ç°°æ±¾£ºV1.0-2024/6/9
*
*****************************************************
*/

uint8_t RxData;			//ÓÃÓÚ½ÓÊÕ´®¿ÚÊı¾İµÄ±äÁ¿
uint8_t move_mode1 = '0';//ÓÃÓÚ½ÓÊÕÀ¶ÑÀ´®¿ÚÊı¾İµÄ±äÁ¿
uint8_t move_mode3 = '0';//ÓÃÓÚ½ÓÊÕÓïÒôÊ¶±ğ´®¿ÚÊı¾İµÄ±äÁ¿
uint8_t move_mode = '0';//¾ö¶¨×´Ì¬±äÁ¿
uint8_t previous_mode = '0';//ÓÃÓÚ±£´æÉÏÒ»´Î½ÓÊÕ´®¿ÚÊı¾İµÄ±äÁ¿
 uint16_t T=100; // ¼ÇÂ¼¾àÀë ¸ø³õÖµ´óÓÚ10cm(²»´¥·¢±ÜÕÏ) 
uint16_t bz_flag;//±ÜÕÏ±êÖ¾Î» ON/OFF Ä¬ÈÏoff
uint16_t hw_flag;//ºìÍâ±êÖ¾Î» ON/OFF Ä¬ÈÏoff
int happiness; //¿ªĞÄÖ¸Êı
int stamina;  //ÌåÁ¦Öµ
uint8_t t1=0 ; //´æ´¢ºìÍâµÄĞÅºÅ Ç°
uint8_t t2=0 ; //´æ´¢ºìÍâµÄĞÅºÅ ºó
uint8_t t3=0 ; //´æ´¢ºìÍâµÄĞÅºÅ ×ó
uint8_t t4=0 ; //´æ´¢ºìÍâµÄĞÅºÅ ÓÒ
uint16_t ff=0; //Á¬ĞøÇ°½ø¼ÆÊı
uint16_t bb=0; //Á¬ĞøºóÍË¼ÆÊı
uint16_t ll=0; //Á¬Ğø×ó×ª¼ÆÊı
uint16_t rr=0; //Á¬ĞøÓÒ×ª¼ÆÊı
static uint8_t continuous_cmd = 0U; /* gait that repeats until a new command */
static uint16_t gait_repeat = 0U;   /* same gait back-to-back counter */

/* OLED face per command, same faces the legacy mode_xxx() functions used. */
static void Face_ShowForCommand(uint8_t cmd)
{
	switch (cmd)
	{
		case 'f': case 'b': case 'l': case 'r':
			OLED_ShowImage(0, 0, 128, 64, BMP2); break;
		case 'w': case 'z': case 'd':
			OLED_ShowImage(0, 0, 128, 64, BMP5); break;
		case '5': case 'q': case '1':
			OLED_ShowImage(0, 0, 128, 64, BMP1); break;
		case 's': case 'o': case 'y':
			OLED_ShowImage(0, 0, 128, 64, BMP12); break;
		default:
			break;
	}
	LED13_Turn();
}

int main(void)
{
	uint32_t now_ms;

	/*Ä£¿é³õÊ¼»¯*/
	LED_Init();			//LED³õÊ¼»¯
	OLED_Init();		//OLED³õÊ¼»¯
	USART1_Init();		//À¶ÑÀ´®¿Ú³õÊ¼»¯
	USART2_Init();		//ÓïÒôºÏ³É´®¿Ú³õÊ¼»¯
	USART3_Init();		//ÓïÒôÊ¶±ğ´®¿Ú³õÊ¼»¯
	CommTask_Init();        //ç¬¬äºŒé˜¶æ®µï¼šåˆå§‹åŒ–é€šä¿¡ä»»åŠ¡
	SafetyTask_Init();     //ç¬¬äºŒé˜¶æ®µï¼šåˆå§‹åŒ–å®‰å…¨ä»»åŠ¡
	ActionTask_Init();      //ç¬¬ä¸‰é˜¶æ®µï¼šåˆå§‹åŒ– PoseStep åŠ¨ä½œçŠ¶æ€æœº
	AudioTask_Init();       //ç¬¬äº”é˜¶æ®µï¼šåˆå§‹åŒ–éé˜»å¡è¯­éŸ³ä»»åŠ¡
	Servo_Init();		//¶æ»ú³õÊ¼»¯
	hongwai_init();     //ºìÍâ³õÊ¼»¯
	UltrasonicWave_Init(); //³¬Éù²¨³õÊ¼»¯
	Timebase_Init();       //1msåä½œå¼è°ƒåº¦æ—¶åŸº
	AppTask_Init();        //åˆå§‹åŒ–ä»»åŠ¡è°ƒåº¦éª¨æ¶
	happiness=200;//³õÊ¼»¯¿ªĞÄÖ¸Êı
	stamina=500;//³õÊ¼»¯ÌåÁ¦Öµ
	bz_flag=0;//±ÜÕÏ±êÖ¾Î» ON/OFF Ä¬ÈÏoff
	hw_flag=0;//ºìÍâ±êÖ¾Î» ON/OFF Ä¬ÈÏoff
	/*½øÈëÄ¬ÈÏ×´Ì¬*/
	OLED_Clear();
	OLED_ShowImage(0, 0, 128, 64, BMP1); //Á¢Õı
 
/**************ÓïÒôºÏ³ÉĞ¾Æ¬ÉèÖÃÃüÁî*********************/
//Ñ¡Ôñ±³¾°ÒôÀÖ2¡£(0£ºÎŞ±³¾°ÒôÀÖ  1-15£º±³¾°ÒôÀÖ¿ÉÑ¡)
//m[0~16]:0±³¾°ÒôÀÖÎª¾²Òô£¬16±³¾°ÒôÀÖÒôÁ¿×î´ó
//v[0~16]:0ÀÊ¶ÁÒôÁ¿Îª¾²Òô£¬16ÀÊ¶ÁÒôÁ¿×î´ó
//t[0~5]:0ÀÊ¶ÁÓïËÙ×îÂı£¬5ÀÊ¶ÁÓïËÙ×î¿ì

	SYN_FrameInfo(0, (uint8_t *)"[v12][m0][t5]ÍúÍú");Delay_s(1);//»½ĞÑºó¹·½ĞÒ»Éù,ÒôÁ¿12ÊÊºÏÑİÊ¾£¬10ÊÊºÏµ÷ÊÔ
	
	Watchdog_Init();       /* after all blocking start-up code */

	while (1)
	{  
		now_ms = Timebase_NowMs();
		AppTask_Run(now_ms); //ç¬¬ä¸€é˜¶æ®µï¼šè¿è¡Œåˆ°æœŸçš„ç©ºä»»åŠ¡æ§½ä½
		if (SafetyTask_ConsumeStopRequest() != 0U)
		{
			move_mode = '5';
			continuous_cmd = 0U;
		}

		/* Read the newest command BEFORE dispatching, so motion commands go
		 * to the non-blocking ActionTask first. Previously the byte was read
		 * after dispatch and fell straight into the blocking legacy mode_xxx()
		 * chain below, which bypassed the PoseStep state machine and then ran
		 * the gait a second time on the next loop. */
		{
			/* ä½“åŠ› <= 0 æ—¶åªæ¥å—ç¡è§‰å‘½ä»¤ï¼›å­—èŠ‚ç…§å¸¸å–å‡ºï¼Œé¿å…æ—§æ•°æ®åœ¨æ¢å¤ä½“åŠ›åè¢«é‡æ”¾ */
			uint8_t rx = 0U;
			move_mode1 = USART1_GetRxData();
			move_mode3 = USART3_GetRxData();
			if (USART1_GetRxFlag()) {          /* Bluetooth has priority over voice */
				rx = move_mode1;
			}
			else if (USART3_GetRxFlag()) {
				rx = move_mode3;
			}
			if ((rx != 0U) && ((stamina > 0) || (rx == '2') || (rx == 'p'))) {
				move_mode = rx;
			}
		}

		if (move_mode != '0')
		{
			if (ActionTask_HandleCommand(move_mode) != 0U)
			{
				PetStats_Apply(move_mode, previous_mode, &gait_repeat, &stamina, &happiness);
				Face_ShowForCommand(move_mode);
				continuous_cmd = PetStats_IsContinuous(move_mode) ? move_mode : 0U;
				previous_mode = move_mode;
				move_mode = '0';
			}
			else
			{
				continuous_cmd = 0U;   /* any other new command stops walking */
			}
		}
		else if ((continuous_cmd != 0U) && (stamina > 0) &&
		         (ActionTask_IsActive() == 0U))
		{
			/* Legacy behaviour: keep walking / dancing until a new command. */
			if (ActionTask_HandleCommand(continuous_cmd) != 0U)
			{
				PetStats_Apply(continuous_cmd, previous_mode, &gait_repeat, &stamina, &happiness);
				previous_mode = continuous_cmd;
			}
		}

		//***ä½“åŠ›å€¼ä¸å¼€å¿ƒå€¼ååº”è®¾ç½®***//
		if(stamina<0)//ÌåÁ¦ÖµĞ¡ÓÚ0£¬Ö´ĞĞ¡°ÄÑÊÜ¡±×´Ì¬
	{
		mode_nanshou();
		stamina=0;
	}
		if(stamina>500)//ÌåÁ¦Öµ²»´óÓÚ1000
		{stamina=500;}
		
		if(happiness<0){//¿ªĞÄÖµĞ¡ÓÚ0ºó£¬Ö´ĞĞ¡°ÄÑÊÜ¡±×´Ì¬£»¸ø³öÔö¼Ó¿ªĞÄÖµµÄÖ¸Áîºó°ÚÍÑÄÑÊÜ×´Ì¬
		happiness=0;
		mode_nanshou();
		}
		if(happiness>200)//¿ªĞÄÖµ²»´óÓÚ1000
		{happiness=200;}
		
		//***ºìÍâĞüÑÂ¼ì²âÄ£¿éÉèÖÃ***//
		if(move_mode == 'H') //ºìÍâ¿ª
		{hw_flag=1;}
		if(move_mode == 'h') //ºìÍâ¹Ø
		{hw_flag=0;
			//ÒÔÏÂ¸øËÄ¸ö´«¸ĞÆ÷Êä³öÇåÁã
			t1=0;t2=0;t3=0;t4=0;
		}
		if(hw_flag){
			if (Edge_detect() != 0U) { continuous_cmd = 0U; } /* cliff: stop walking */
		}
		
	   //***³¬Éù±ÜÕÏÄ£¿éÉèÖÃ***//
		if(move_mode == 'x')
		{
			bz_flag=1;
			SensorTask_SetEnabled(1U);
		}
		if(move_mode == 'c')
		{
			bz_flag=0;
			SensorTask_SetEnabled(0U);
			T=100;
		}
		/* SensorTask runs from AppTask; no blocking Bizhang call here. */
		
		
		//***»¥¶¯ÉèÖÃ***//
		/* Only commands that ActionTask does not own reach here. All motion
		 * commands are accepted by ActionTask above (see test_state_machine.c,
		 * "every protocol motion byte is accepted"), so the old blocking
		 * mode_forward()/mode_dance()/... branches were unreachable and are gone. */
		switch (move_mode)
		{
			case 'n': mode_nanshou();   break;   /* éš¾å— */
			case 'K': mode_happiness(); break;   /* æ’­æŠ¥å¼€å¿ƒå€¼ */
			case 'T': mode_stamina();   break;   /* æ’­æŠ¥ä½“åŠ›å€¼ */
			case 'Z': mode_index();     break;   /* æ’­æŠ¥å¼€å¿ƒå€¼+ä½“åŠ›å€¼ */
			case 'H': case 'h': case 'x': case 'c':
				move_mode = '0';                 /* mode switches handled above */
				break;
			default:
				break;
		}

		Watchdog_Feed();
	}
}
