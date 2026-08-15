#ifndef TEST_STM32F10X_H
#define TEST_STM32F10X_H

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef struct { int unused; } TIM_TypeDef;
typedef struct
{
    uint16_t TIM_ClockDivision;
    uint16_t TIM_CounterMode;
    uint16_t TIM_Period;
    uint16_t TIM_Prescaler;
    uint16_t TIM_RepetitionCounter;
} TIM_TimeBaseInitTypeDef;
typedef struct
{
    uint8_t NVIC_IRQChannel;
    uint8_t NVIC_IRQChannelPreemptionPriority;
    uint8_t NVIC_IRQChannelSubPriority;
    uint8_t NVIC_IRQChannelCmd;
} NVIC_InitTypeDef;

#define TIM4 ((TIM_TypeDef *)0)
#define RCC_APB1Periph_TIM4 1U
#define TIM_CKD_DIV1 0U
#define TIM_CounterMode_Up 0U
#define TIM_FLAG_Update 1U
#define TIM_IT_Update 1U
#define TIM4_IRQn 30U
#define ENABLE 1U
#define DISABLE 0U
#define SET 1U

static inline void RCC_APB1PeriphClockCmd(uint32_t peripheral, uint32_t state)
{ (void)peripheral; (void)state; }
static inline void TIM_InternalClockConfig(TIM_TypeDef *tim)
{ (void)tim; }
static inline void TIM_TimeBaseInit(TIM_TypeDef *tim, TIM_TimeBaseInitTypeDef *init)
{ (void)tim; (void)init; }
static inline void TIM_ClearFlag(TIM_TypeDef *tim, uint16_t flag)
{ (void)tim; (void)flag; }
static inline void TIM_ITConfig(TIM_TypeDef *tim, uint16_t it, uint32_t state)
{ (void)tim; (void)it; (void)state; }
static inline void NVIC_Init(NVIC_InitTypeDef *init)
{ (void)init; }
static inline void TIM_Cmd(TIM_TypeDef *tim, uint32_t state)
{ (void)tim; (void)state; }
static inline uint32_t TIM_GetITStatus(TIM_TypeDef *tim, uint16_t it)
{ (void)tim; (void)it; return 0U; }
static inline void TIM_ClearITPendingBit(TIM_TypeDef *tim, uint16_t it)
{ (void)tim; (void)it; }

#endif
