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
	
	while (1)
	{  
		now_ms = Timebase_NowMs();
		AppTask_Run(now_ms); //ç¬¬ä¸€é˜¶æ®µï¼šè¿è¡Œåˆ°æœŸçš„ç©ºä»»åŠ¡æ§½ä½
		if (SafetyTask_ConsumeStopRequest() != 0U)
		{
			move_mode = '5';
			previous_mode = '5';
		}
		if (ActionTask_HandleCommand(move_mode) != 0U)
		{
			move_mode = '0';
		}



		//***ÌåÁ¦ÖµÓë¿ªĞÄÖµ·´Ó¦ÉèÖÃ***//
		if(stamina>0)//ÌåÁ¦Öµ´óÓÚÁã²Å»áÌıÁî
	{
		move_mode1 = USART1_GetRxData();		//»ñÈ¡À¶ÑÀ´®¿Ú½ÓÊÕµÄÊı¾İ
		move_mode3 = USART3_GetRxData();		//»ñÈ¡ÓïÒôÊ¶±ğ´®¿Ú½ÓÊÕµÄÊı¾İ
		if (USART1_GetRxFlag()) {//ÉèÖÃÀ¶ÑÀ½ÓÊÕÓÅÏÈ¼¶´óÓÚÓïÒôÊ¶±ğ
			move_mode = move_mode1;
		}
		else if (USART3_GetRxFlag()){
			move_mode = move_mode3;
		}
	}
		if(stamina<0)//ÌåÁ¦ÖµĞ¡ÓÚ0£¬Ö´ĞĞ¡°ÄÑÊÜ¡±×´Ì¬
	{
		mode_nanshou();
		stamina=0;
	}
		if(stamina==0)//ÌåÁ¦ÖµÎª0ºó£¬Ö»½ÓÊÜË¯¾õÖ¸Áî
	{
		if (USART1_GetRxData() == '2' || USART3_GetRxData() == '2')
		{mode_sleepwo();}
		else if (USART1_GetRxData() == 'p' || USART3_GetRxData() == 'p')
		{mode_sleeppa();}
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
			Edge_detect();//´ò¿ªºìÍâĞüÑÂ¼ì²âÄ£Ê½	
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
		if (move_mode == 'f') { //Ç°½ø
			mode_forward();
		} else if (move_mode == 'b') { //ºóÍË
			mode_behind();
		} else if (move_mode == 'l') { //×ó×ª
			mode_left();
		} else if (move_mode == 'r') { //ÓÒ×ª
			mode_right();
		} else if (move_mode == 'w') { //Ç°ºóÒ¡°Ú
			mode_swing_qianhou();
		} else if (move_mode == 'z') { //×óÓÒÒ¡°Ú
			mode_swing_zuoyou();
		} else if (move_mode == 'd') { //ÌøÎè
			mode_dance();
		} else if (move_mode == '5') { //Á¢Õı
			mode_stand();
		} else if (move_mode == 'q' && previous_mode != '0') { //ÆğÉí
			mode_slowstand();
		} else if (move_mode == 's' && previous_mode != 's') { //×øÏÂ
			mode_strech();
		} else if (move_mode == 'j') { //½»ÌæÌ§ÊÖ
			mode_twohands();
		} else if (move_mode == 'y') { //ÉìÀÁÑü
			mode_lanyao();
		} else if (move_mode == '1') { //Ì§Í·
			mode_headup();
		} else if (move_mode == 'p' && previous_mode != 'p') { //Å¿ÏÂË¯¾õ
			mode_sleeppa();
		} else if (move_mode == '2' && previous_mode != '2') { //ÎÔÏÂË¯¾õ
			mode_sleepwo();
		} else if (move_mode == 'n') { //ÄÑÊÜ
			mode_nanshou();
		} else if (move_mode == 'B') { //±í°×
			mode_biaobai();
		} else if (move_mode == 'Y') { //±³ÔªËØÖÜÆÚ±í
			mode_yuansu();
		} else if (move_mode == 'X') { //±³Ğ£Ñµ
			mode_xiaoxun();
		} else if (move_mode == 'W') { //ÎªÊÀ½çÖ®¹â
			mode_world();
		} else if (move_mode == 'o') { //´òÕĞºô
			mode_hello();
		} else if (move_mode == 'U') { //±»½Ğ¡°Ğ¡´ô¡±
			mode_xiaodai();
		} else if (move_mode == 'K') { //Õ¹Ê¾¿ªĞÄÖµ
			mode_happiness();
		} else if (move_mode == 'T') { //Õ¹Ê¾ÌåÁ¦Öµ
			mode_stamina();
		} else if (move_mode == 'Z') { //Õ¹Ê¾¿ªĞÄÖµºÍÌåÁ¦Öµ
			mode_index();
		} 


	}
}
