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

/*
 * Feature set.
 *   MANBO_LITE: low-cost build - voice commands (SU-03T on USART3), motion,
 *   OLED faces and voice replies through the SU-03T's own speaker.
 *   No SYN6288, no ultrasonic / IR, no battery sensing, no stamina game.
 *   Walking stops by itself after MANBO_WALK_TIMEOUT_MS in case the "stop"
 *   phrase is not recognised. Use with the lite board in hardware/lite/.
 */
#ifdef MANBO_LITE
  #define MANBO_HAS_OLED       1
  #define MANBO_HAS_SPEECH     0      /* no SYN6288 ...                     */
  #define MANBO_HAS_VOICE_REPLY 1     /* ... replies via SU-03T frames instead */
  #define MANBO_HAS_SENSORS    0
  #define MANBO_HAS_PET_STATS  0
  #ifndef MANBO_WALK_TIMEOUT_MS
    #define MANBO_WALK_TIMEOUT_MS 10000U
  #endif
#else
  #define MANBO_HAS_OLED       1
  #define MANBO_HAS_SPEECH     1
  #define MANBO_HAS_VOICE_REPLY 1
  #define MANBO_HAS_SENSORS    1
  #define MANBO_HAS_PET_STATS  1
  #ifndef MANBO_WALK_TIMEOUT_MS
    #define MANBO_WALK_TIMEOUT_MS 0U      /* 0 = walk until the next command */
  #endif
#endif

#if defined(MANBO_LITE) && defined(MANBO_BOARD_V2)
  #error "MANBO_LITE uses the lite board; do not combine with MANBO_BOARD_V2"
#endif

#endif /* BOARD_CONFIG_H */
