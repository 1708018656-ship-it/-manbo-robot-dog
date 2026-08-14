#include "syn6288.h"
#include "usart2.h"
#include "string.h"
#include "delay.h"
//***语音合成***//

#define SYN_FRAME_BUFFER_SIZE 50U
#define SYN_MAX_TEXT_LENGTH   (SYN_FRAME_BUFFER_SIZE - 6U) /* 5-byte header + ECC */

//Music:选择背景音乐。0:无背景音乐，1~15：选择背景音乐
void SYN_FrameInfo(u8 Music, u8 *HZdata)
{
  /****************需要发送的文本**********************************/
  unsigned char Frame_Info[SYN_FRAME_BUFFER_SIZE];
  unsigned int HZ_Length;
  unsigned char ecc = 0;                 //定义校验字节
  unsigned int i = 0;

  if (HZdata == 0)
  {
    return;
  }

  HZ_Length = strlen((char *)HZdata);
  if (HZ_Length > SYN_MAX_TEXT_LENGTH)
  {
    HZ_Length = SYN_MAX_TEXT_LENGTH;
  }

  /*****************帧固定配置信息**************************************/
  Frame_Info[0] = 0xFD;                  //构造帧头FD
  Frame_Info[1] = 0x00;                  //构造数据区吵度的高字节
  Frame_Info[2] = HZ_Length + 3;         //构造数据区吵度的低字节
  Frame_Info[3] = 0x01;                  //构造命令字：合成播放命令
  Frame_Info[4] = 0x01 | Music << 4;    //构造命令参数：背景音乐设定

  /*******************校验码计算***************************************/
  for (i = 0; i < 5; i++)
  {
    ecc = ecc ^ Frame_Info[i];
  }

  for (i = 0; i < HZ_Length; i++)
  {
    ecc = ecc ^ HZdata[i];
  }

  /*******************发送帧信息***************************************/
  memcpy(&Frame_Info[5], HZdata, HZ_Length);
  Frame_Info[5 + HZ_Length] = ecc;
  USART2_SendString(Frame_Info, 5 + HZ_Length + 1);
}


/***********************************************************
* 名    称： YS_SYN_Set(u8 *Info_data)
* 功    能： 主函数	程序入口
* 入口参数： *Info_data:固定的配置信息变量
* 出口参数：
* 说    明：本函数用于配置，停止合成、暂停合成等设置 ，默认波特率9600bps。
* 调用方法：通过定义的相关数组进行配置。
**********************************************************/
void YS_SYN_Set(u8 *Info_data)
{
  u8 Com_Len;

  if (Info_data == 0)
  {
    return;
  }

  Com_Len = strlen((char *)Info_data);
  USART2_SendString(Info_data, Com_Len);
}
