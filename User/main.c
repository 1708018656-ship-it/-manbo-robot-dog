#include "stm32f10x.h"                  // Device header
#include "stdlib.h"
#include <stdio.h>
#include "LED.h"
#include "Delay.h"
#include "OLED.h"
#include "Servo.h"
#include "PWM.h"
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
#include "BatteryTask.h"
#include "BoardConfig.h"
#include "Voice.h"

/****************************************************
*
*程序名称：电子桌面宠物“小呆”
*当前版本：V1.0-2024/6/9
*
*****************************************************
*/

uint8_t RxData;			//用于接收串口数据的变量
uint8_t move_mode = '0';//决定状态变量
uint8_t previous_mode = '0';//用于保存上一次接收串口数据的变量
 uint16_t T=100; // 记录距离 给初值大于10cm(不触发避障) 
uint16_t bz_flag;//避障标志位 ON/OFF 默认off
uint16_t hw_flag;//红外标志位 ON/OFF 默认off
int happiness; //开心指数
int stamina;  //体力值
uint8_t t1=0 ; //存储红外的信号 前
uint8_t t2=0 ; //存储红外的信号 后
uint8_t t3=0 ; //存储红外的信号 左
uint8_t t4=0 ; //存储红外的信号 右
static uint8_t continuous_cmd = 0U; /* gait that repeats until a new command */
static uint16_t gait_repeat = 0U;   /* same gait back-to-back counter */
static uint32_t continuous_start_ms = 0U; /* for MANBO_WALK_TIMEOUT_MS */

/* OLED face per command, same faces the legacy mode_xxx() functions used. */
static void Face_ShowForCommand(uint8_t cmd)
{
#if MANBO_HAS_OLED
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
#else
	(void)cmd;
#endif
	LED13_Turn();   /* lite build: the on-board LED blinks on every command */
}

/* Motion is refused when stamina is used up or the battery is critical. */
static uint8_t MotionAllowed(void)
{
#if MANBO_HAS_PET_STATS
	return ((stamina > 0) && (BatteryTask_Level() != BATTERY_CRITICAL)) ? 1U : 0U;
#else
	return (BatteryTask_Level() != BATTERY_CRITICAL) ? 1U : 0U;
#endif
}

/* Board V2 only (original wiring always reports BATTERY_UNKNOWN). */
static void Battery_React(void)
{
	BatteryLevel level;

	if (BatteryTask_ConsumeChange(&level) == 0U)
	{
		return;
	}
	if (level == BATTERY_LOW)
	{
		(void)AudioTask_RequestText("[v12][m0][t5]" "\xB5\xE7\xC1\xBF\xB2\xBB\xD7\xE3\xA3\xAC\xC7\xEB\xB3\xE4\xB5\xE7" /* 电量不足，请充电 */);
	}
	else if (level == BATTERY_CRITICAL)
	{
		continuous_cmd = 0U;
		(void)ActionTask_RequestMotion(ACTION_SLEEP_PRONE);   /* lowest servo load */
		OLED_ShowImage(0, 0, 128, 64, BMP6);                  /* sleeping face */
		(void)AudioTask_RequestText("[v12][m0][t5]" "\xB5\xE7\xC1\xBF\xB9\xFD\xB5\xCD\xA3\xAC\xD0\xA1\xB4\xF4\xD2\xAA\xD0\xDD\xCF\xA2\xC1\xCB" /* 电量过低，小呆要休息了 */);
	}
}

int main(void)
{
	uint32_t now_ms;

	/*模块初始化*/
	LED_Init();			//LED初始化
#if MANBO_HAS_OLED
	OLED_Init();		//OLED初始化
#endif
	USART1_Init();		//蓝牙串口初始化
#if MANBO_HAS_SPEECH
	USART2_Init();		//语音合成串口初始化
#endif
	USART3_Init();		//语音识别串口初始化
	CommTask_Init();        //第二阶段：初始化通信任务
	SafetyTask_Init();     //第二阶段：初始化安全任务
	ActionTask_Init();      //第三阶段：初始化 PoseStep 动作状态机
	AudioTask_Init();       //第五阶段：初始化非阻塞语音任务
	Servo_Init();		//舵机初始化
#if MANBO_HAS_SENSORS
	hongwai_init();     //红外初始化
	UltrasonicWave_Init(); //超声波初始化
#endif
	Timebase_Init();       //1ms协作式调度时基
	AppTask_Init();        //初始化任务调度骨架
	happiness=200;//初始化开心指数
	stamina=500;//初始化体力值
	bz_flag=0;//避障标志位 ON/OFF 默认off
	hw_flag=0;//红外标志位 ON/OFF 默认off
	/*进入默认状态*/
#if MANBO_HAS_OLED
	OLED_Clear();
	OLED_ShowImage(0, 0, 128, 64, BMP1); //立正
	OLED_Update();          /* boot: show the face now (normally AppTask_Ui refreshes) */
#endif
 
/**************语音合成芯片设置命令*********************/
//选择背景音乐2。(0：无背景音乐  1-15：背景音乐可选)
//m[0~16]:0背景音乐为静音，16背景音乐音量最大
//v[0~16]:0朗读音量为静音，16朗读音量最大
//t[0~5]:0朗读语速最慢，5朗读语速最快

	/* After a watchdog reset skip the 1 s greeting: recover quickly, and do
	 * not bark on every reset if something keeps tripping the watchdog. */
#if MANBO_HAS_SPEECH
	if (Watchdog_CausedLastReset() == 0U)
	{
		SYN_FrameInfo(0, (uint8_t *)"[v12][m0][t5]\xCD\xFA\xCD\xFA" /* 旺旺 */);
		Delay_s(1);   //唤醒后狗叫一声,音量12适合演示，10适合调试
	}
#else
	if (Watchdog_CausedLastReset() == 0U)
	{
		(void)Voice_Play(VOICE_BOOT);    /* "小呆来啦" from the SU-03T speaker */
	}
	(void)ActionTask_RequestMotion(ACTION_STAND);
#endif
	
	Watchdog_Init();       /* after all blocking start-up code */

	while (1)
	{  
		now_ms = Timebase_NowMs();
		AppTask_Run(now_ms); //第一阶段：运行到期的空任务槽位
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
			/* One queued command per loop (CommTask FIFO, Bluetooth before voice).
			 * 体力耗尽或电池电量过低时只接受睡觉；急停 '5' 走 SafetyTask，不经过这里。 */
			uint8_t rx;
			if (CommTask_PopCommand(&rx) != 0U)
			{
				if ((MotionAllowed() != 0U) || (rx == '2') || (rx == 'p')) {
					move_mode = rx;
				}
			}
		}

		if (move_mode != '0')
		{
			if (ActionTask_HandleCommand(move_mode) != 0U)
			{
				PetStats_Apply(move_mode, previous_mode, &gait_repeat, &stamina, &happiness);
				Face_ShowForCommand(move_mode);
				continuous_cmd = PetStats_IsContinuous(move_mode) ? move_mode : 0U;
				continuous_start_ms = now_ms;
				previous_mode = move_mode;
				move_mode = '0';
			}
			else
			{
				continuous_cmd = 0U;   /* any other new command stops walking */
			}
		}
		else if ((MANBO_WALK_TIMEOUT_MS != 0U) && (continuous_cmd != 0U) &&
		         ((uint32_t)(now_ms - continuous_start_ms) >= MANBO_WALK_TIMEOUT_MS))
		{
			/* safety net for voice control: stop if "停" was not recognised */
			continuous_cmd = 0U;
			(void)ActionTask_RequestMotion(ACTION_STAND);
			(void)Voice_Play(VOICE_AUTO_STOP);
			Face_ShowForCommand('5');
		}
		else if ((continuous_cmd != 0U) && (MotionAllowed() != 0U) &&
		         (ActionTask_IsActive() == 0U))
		{
			/* Legacy behaviour: keep walking / dancing until a new command. */
			if (ActionTask_HandleCommand(continuous_cmd) != 0U)
			{
				PetStats_Apply(continuous_cmd, previous_mode, &gait_repeat, &stamina, &happiness);
				previous_mode = continuous_cmd;
			}
		}

		Battery_React();

#if MANBO_HAS_PET_STATS
		//***体力值与开心值反应设置***//
		if(stamina<0)//体力值小于0，执行“难受”状态
	{
		mode_nanshou();
		stamina=0;
	}
		if(stamina>500)//体力值不大于1000
		{stamina=500;}
		
		if(happiness<0){//开心值小于0后，执行“难受”状态；给出增加开心值的指令后摆脱难受状态
		happiness=0;
		mode_nanshou();
		}
		if(happiness>200)//开心值不大于1000
		{happiness=200;}
		
#endif
#if MANBO_HAS_SENSORS
		//***红外悬崖检测模块设置***//
		if(move_mode == 'H') //红外开
		{hw_flag=1;}
		if(move_mode == 'h') //红外关
		{hw_flag=0;
			//以下给四个传感器输出清零
			t1=0;t2=0;t3=0;t4=0;
		}
		if(hw_flag){
			if (Edge_detect() != 0U) { continuous_cmd = 0U; } /* cliff: stop walking */
		}
		
	   //***超声避障模块设置***//
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
#endif
		/* SensorTask runs from AppTask; no blocking Bizhang call here. */
		
		
		//***互动设置***//
		/* Only commands that ActionTask does not own reach here. All motion
		 * commands are accepted by ActionTask above (see test_state_machine.c,
		 * "every protocol motion byte is accepted"), so the old blocking
		 * mode_forward()/mode_dance()/... branches were unreachable and are gone. */
		switch (move_mode)
		{
#if MANBO_HAS_PET_STATS
			case 'n': mode_nanshou();   break;   /* 难受 */
			case 'K': mode_happiness(); break;   /* 播报开心值 */
			case 'T': mode_stamina();   break;   /* 播报体力值 */
			case 'Z': mode_index();     break;   /* 播报开心值+体力值 */
#endif
			case 'H': case 'h': case 'x': case 'c':
				move_mode = '0';                 /* mode switches handled above */
				break;
			default:
				break;
		}

		Watchdog_Feed();
	}
}
