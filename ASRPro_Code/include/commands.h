/**
  ******************************************************************************
  * @file    commands.h
  * @author  Manbo Robot Dog Team
  * @version V1.0
  * @date    2025-10-31
  * @brief   ESP32与STM32通信命令定义
  *          定义了通过UART发送给STM32的单字符命令
  ******************************************************************************
  */

#ifndef __COMMANDS_H
#define __COMMANDS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 运动控制命令 ----------------------------------------------------------------*/
#define CMD_FORWARD         'f'    /**< 前进 */
#define CMD_BACKWARD        'b'    /**< 后退 */
#define CMD_TURN_LEFT       'l'    /**< 左转 */
#define CMD_TURN_RIGHT      'r'    /**< 右转 */
#define CMD_STOP            '5'    /**< 停止 */

/* 姿态控制命令 ----------------------------------------------------------------
 * 必须与 STM32 端 User/ActionTask.c 的 ActionTask_HandleCommand() 一致，
 * 完整协议见 docs/protocol.md。旧版本里 CMD_STAND='1'、CMD_SIT='3' 是错的：
 * STM32 上 '1' 是抬头，'3' 没有任何处理。 */
#define CMD_STAND           'q'    /**< 起身/站立 */
#define CMD_SIT             's'    /**< 坐下擦脸 */
#define CMD_LIE             'p'    /**< 趴下睡觉 */
#define CMD_SLEEP_SIDE      '2'    /**< 卧下睡觉 */
#define CMD_HEAD_UP         '1'    /**< 抬头 */
#define CMD_DANCE           'd'    /**< 跳舞 */
#define CMD_SWING_FB        'w'    /**< 前后摇摆 */
#define CMD_SWING_LR        'z'    /**< 左右摇摆 */

/* 动作命令 --------------------------------------------------------------------*/
#define CMD_SHAKE_HAND      'j'    /**< 握手 = 交替抬手（STM32 没有单独的握手动作） */
#define CMD_HELLO           'o'    /**< 打招呼 */
#define CMD_LAZY            'y'    /**< 伸懒腰 */
#define CMD_STATUS          'Z'    /**< 播报开心值+体力值 */
#define CMD_HAPPINESS       'K'    /**< 播报开心值 */
#define CMD_STAMINA         'T'    /**< 播报体力值 */
#define CMD_DECLARE_LOVE    'B'    /**< 表白 */
#define CMD_ELEMENTS        'Y'    /**< 背元素周期表 */
#define CMD_MOTTO           'X'    /**< 背校训 */
#define CMD_WORLD_LIGHT     'W'    /**< 为世界之光 */
#define CMD_NAME_CALLED     'U'    /**< 被叫"小呆" */

/* 避障模式命令 ----------------------------------------------------------------*/
#define CMD_ULTRASONIC_ON   'x'    /**< 打开超声波避障 */
#define CMD_ULTRASONIC_OFF  'c'    /**< 关闭超声波避障 */
#define CMD_INFRARED_ON     'H'    /**< 打开红外模式 */
#define CMD_INFRARED_OFF    'h'    /**< 关闭红外模式 */

/* 命令映射表结构 --------------------------------------------------------------*/
typedef struct {
    uint32_t snid;        /**< 语音识别ID */
    char command;         /**< 对应的命令字符 */
    const char* desc;     /**< 命令描述 */
} CommandMap_t;

/* 命令映射表声明 --------------------------------------------------------------*/
extern const CommandMap_t g_CommandMap[];
extern const uint8_t g_CommandMapSize;

/**
 * @brief  根据语音识别ID获取对应的命令字符
 * @param  snid: 语音识别ID
 * @param  cmd: 输出的命令字符指针
 * @retval true: 找到对应命令, false: 未找到
 */
bool GetCommandBySnid(uint32_t snid, char* cmd);

/**
 * @brief  获取命令描述
 * @param  snid: 语音识别ID
 * @retval 命令描述字符串，未找到返回NULL
 */
const char* GetCommandDescription(uint32_t snid);

#ifdef __cplusplus
}
#endif

#endif /* __COMMANDS_H */
