# Manbo carrier V2 - Blue Pill pin map

Generated from `hardware/gen/design.py`. Firmware: build with `MANBO_BOARD_V2` (`make V2=1`, or add the define in Keil).

| Blue Pill pin | STM32 | Net | Goes to |
|---|---|---|---|
| 1 | PB12 | IR_FRONT | J11.3 |
| 2 | PB13 | IR_BACK | J12.3 |
| 3 | PB14 | IR_LEFT | J13.3 |
| 4 | PB15 | IR_RIGHT | J14.3 |
| 5 | PA8 | - |  |
| 6 | PA9 | BT_RXD | J6.5 |
| 7 | PA10 | BT_TXD | J6.4 |
| 8 | PA11 | - |  |
| 9 | PA12 | - |  |
| 10 | PA15 | - |  |
| 11 | PB3 | - |  |
| 12 | PB4 | - |  |
| 13 | PB5 | - |  |
| 14 | PB6 | - |  |
| 15 | PB7 | - |  |
| 16 | PB8 | OLED_SCL | J9.3 |
| 17 | PB9 | OLED_SDA | J9.4 |
| 18 | 5V | +5V | (power) |
| 19 | GND | GND | (power) |
| 20 | 3V3 | +3V3 | (power) |
| 21 | GND | GND | (power) |
| 22 | GND | GND | (power) |
| 23 | 3V3 | +3V3 | (power) |
| 24 | NRST | - |  |
| 25 | PB11 | ASR_TXD | J8.4 |
| 26 | PB10 | ASR_RXD | J8.3 |
| 27 | PB1 | SERVO4_PWM | J5.3 |
| 28 | PB0 | SERVO3_PWM | J4.3 |
| 29 | PA7 | SERVO2_PWM | J3.3 |
| 30 | PA6 | SERVO1_PWM | J2.3 |
| 31 | PA5 | - |  |
| 32 | PA4 | VBAT_SENSE | R7.2, R8.1, C5.1 |
| 33 | PA3 | SYN_TXD | J7.4 |
| 34 | PA2 | SYN_RXD | J7.3 |
| 35 | PA1 | US_ECHO_3V3 | R5.2, R6.1 |
| 36 | PA0 | US_TRIG | J10.2 |
| 37 | PC15 | - |  |
| 38 | PC14 | - |  |
| 39 | PC13 | - |  |
| 40 | VBAT | - |  |
