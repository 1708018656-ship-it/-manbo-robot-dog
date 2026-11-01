#ifndef __HONGWAI_H
#define __HONGWAI_H

#include <stdint.h>

void hongwai_init(void);
uint8_t hongwai_get_1(void);
uint8_t hongwai_get_2(void);
uint8_t hongwai_get_3(void);
uint8_t hongwai_get_4(void);
uint8_t Edge_detect(void); /* non-blocking; returns 1 when avoidance was triggered */

#endif
