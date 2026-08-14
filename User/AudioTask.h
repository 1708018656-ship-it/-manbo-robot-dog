#ifndef __AUDIO_TASK_H
#define __AUDIO_TASK_H

#include "stm32f10x.h"

void AudioTask_Init(void);
void AudioTask_Run(uint32_t now_ms);
uint8_t AudioTask_RequestText(const char *text);
uint8_t AudioTask_IsBusy(void);

#endif
