#ifndef __BYTE_RING_H
#define __BYTE_RING_H

#include "stm32f10x.h"

#define BYTE_RING_CAPACITY 16U

typedef struct
{
    volatile uint8_t data[BYTE_RING_CAPACITY];
    volatile uint8_t head;
    volatile uint8_t tail;
    volatile uint8_t overflow;
} ByteRing;

void ByteRing_Init(ByteRing *ring);
void ByteRing_PushFromIsr(ByteRing *ring, uint8_t value);
uint8_t ByteRing_Pop(ByteRing *ring, uint8_t *value);
uint8_t ByteRing_HasOverflowed(ByteRing *ring);

#endif
