#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

/*
 * Board pin configuration.
 *
 *   MANBO_BOARD_V2 undefined (default): original hand-wired build, pins
 *     exactly as in the upstream project.
 *   MANBO_BOARD_V2 defined: the new KiCad carrier board in hardware/kicad.
 *     - IR sensors moved off PA11/PA12 (USB D-/D+; Blue Pill has a pull-up
 *       on PA12 that biases the reading) to 5V-tolerant PB12..PB15.
 *     - PA4 is freed for battery-voltage sensing (ADC1_IN4).
 *
 * Define MANBO_BOARD_V2 in Keil: Options for Target -> C/C++ -> Define.
 */

#include "stm32f10x.h"

#ifdef MANBO_BOARD_V2
  #define IR_GPIO_CLK      RCC_APB2Periph_GPIOB
  #define IR_FRONT_PORT    GPIOB
  #define IR_FRONT_PIN     GPIO_Pin_12
  #define IR_BACK_PORT     GPIOB
  #define IR_BACK_PIN      GPIO_Pin_13
  #define IR_LEFT_PORT     GPIOB
  #define IR_LEFT_PIN      GPIO_Pin_14
  #define IR_RIGHT_PORT    GPIOB
  #define IR_RIGHT_PIN     GPIO_Pin_15
  #define BOARD_HAS_VBAT_SENSE 1
  #define VBAT_ADC_PORT    GPIOA
  #define VBAT_ADC_PIN     GPIO_Pin_4
  #define VBAT_ADC_CHANNEL ADC_Channel_4
#else
  #define IR_GPIO_CLK      RCC_APB2Periph_GPIOA
  #define IR_FRONT_PORT    GPIOA
  #define IR_FRONT_PIN     GPIO_Pin_11
  #define IR_BACK_PORT     GPIOA
  #define IR_BACK_PIN      GPIO_Pin_4
  #define IR_LEFT_PORT     GPIOA
  #define IR_LEFT_PIN      GPIO_Pin_12
  #define IR_RIGHT_PORT    GPIOA
  #define IR_RIGHT_PIN     GPIO_Pin_5
  #define BOARD_HAS_VBAT_SENSE 0
#endif

#endif /* BOARD_CONFIG_H */
