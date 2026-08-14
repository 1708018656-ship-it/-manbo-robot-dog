#include "ByteRing.h"

void ByteRing_Init(ByteRing *ring)
{
    uint8_t i;

    if (ring == 0)
    {
        return;
    }

    ring->head = 0U;
    ring->tail = 0U;
    ring->overflow = 0U;

    for (i = 0U; i < BYTE_RING_CAPACITY; ++i)
    {
        ring->data[i] = 0U;
    }
}

void ByteRing_PushFromIsr(ByteRing *ring, uint8_t value)
{
    uint8_t next_head;

    if (ring == 0)
    {
        return;
    }

    next_head = (uint8_t)((ring->head + 1U) % BYTE_RING_CAPACITY);
    if (next_head == ring->tail)
    {
        ring->overflow = 1U;
        return;
    }

    ring->data[ring->head] = value;
    ring->head = next_head;
}

uint8_t ByteRing_Pop(ByteRing *ring, uint8_t *value)
{
    if ((ring == 0) || (value == 0) || (ring->tail == ring->head))
    {
        return 0U;
    }

    *value = ring->data[ring->tail];
    ring->tail = (uint8_t)((ring->tail + 1U) % BYTE_RING_CAPACITY);
    return 1U;
}

uint8_t ByteRing_HasOverflowed(ByteRing *ring)
{
    uint8_t overflowed;

    if (ring == 0)
    {
        return 0U;
    }

    overflowed = ring->overflow;
    ring->overflow = 0U;
    return overflowed;
}
