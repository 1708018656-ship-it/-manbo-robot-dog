/* Minimal GPIO stub for SensorTask host tests. */
#ifndef TEST_SENSOR_STM32F10X_H
#define TEST_SENSOR_STM32F10X_H
#include <stdint.h>
typedef uint8_t u8;
typedef struct { int unused; } GPIO_TypeDef;
#define GPIOA ((GPIO_TypeDef *)0)
#define GPIO_Pin_0 0x0001U
#define GPIO_Pin_1 0x0002U
#define RESET 0U
#define SET 1U
extern uint8_t g_echo_level;
extern unsigned g_trigger_count;
static inline uint8_t GPIO_ReadInputDataBit(GPIO_TypeDef *p, uint16_t pin) { (void)p; (void)pin; return g_echo_level; }
static inline void GPIO_SetBits(GPIO_TypeDef *p, uint16_t pin) { (void)p; (void)pin; g_trigger_count++; }
static inline void GPIO_ResetBits(GPIO_TypeDef *p, uint16_t pin) { (void)p; (void)pin; }
#endif
