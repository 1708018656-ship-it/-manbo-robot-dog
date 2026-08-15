#include <stdio.h>

#include "ByteRing.h"
#include "Timebase.h"

static unsigned g_tests;
static unsigned g_failures;

#define EXPECT_TRUE(condition, message) do { \
    g_tests++; \
    if (!(condition)) { \
        g_failures++; \
        fprintf(stderr, "FAIL: %s (%s:%d)\n", message, __FILE__, __LINE__); \
    } \
} while (0)

#define EXPECT_EQ_UINT(actual, expected, message) \
    EXPECT_TRUE((actual) == (expected), message)

static void test_ring_null_and_empty_inputs(void)
{
    ByteRing ring;
    uint8_t value = 0xA5U;

    ByteRing_Init(0);
    ByteRing_PushFromIsr(0, 1U);
    EXPECT_EQ_UINT(ByteRing_Pop(0, &value), 0U,
                   "null ring cannot be popped");
    EXPECT_EQ_UINT(ByteRing_Pop(&ring, 0), 0U,
                   "null output cannot be popped");
    EXPECT_EQ_UINT(ByteRing_HasOverflowed(0), 0U,
                   "null ring has no overflow status");

    ByteRing_Init(&ring);
    EXPECT_EQ_UINT(ByteRing_Pop(&ring, &value), 0U,
                   "new ring is empty");
    EXPECT_EQ_UINT(ByteRing_HasOverflowed(&ring), 0U,
                   "new ring has no overflow");
}

static void test_ring_capacity_overflow_and_order(void)
{
    ByteRing ring;
    uint8_t value;
    uint8_t i;

    ByteRing_Init(&ring);
    for (i = 0U; i < (BYTE_RING_CAPACITY - 1U); ++i)
    {
        ByteRing_PushFromIsr(&ring, i);
    }
    ByteRing_PushFromIsr(&ring, 0xEEU);

    EXPECT_EQ_UINT(ByteRing_HasOverflowed(&ring), 1U,
                   "pushing into a full ring sets overflow");
    EXPECT_EQ_UINT(ByteRing_HasOverflowed(&ring), 0U,
                   "reading overflow status clears it");

    for (i = 0U; i < (BYTE_RING_CAPACITY - 1U); ++i)
    {
        EXPECT_EQ_UINT(ByteRing_Pop(&ring, &value), 1U,
                       "all capacity-minus-one entries can be popped");
        EXPECT_EQ_UINT(value, i,
                       "full ring preserves FIFO ordering");
    }
    EXPECT_EQ_UINT(ByteRing_Pop(&ring, &value), 0U,
                   "ring is empty after all entries are popped");
}

static void test_ring_index_wraparound(void)
{
    ByteRing ring;
    uint8_t value;
    uint8_t i;

    ByteRing_Init(&ring);
    for (i = 0U; i < 15U; ++i)
    {
        ByteRing_PushFromIsr(&ring, i);
    }
    for (i = 0U; i < 8U; ++i)
    {
        EXPECT_EQ_UINT(ByteRing_Pop(&ring, &value), 1U,
                       "initial entries pop before index wrap");
        EXPECT_EQ_UINT(value, i,
                       "initial wrapped sequence remains ordered");
    }
    for (i = 0U; i < 8U; ++i)
    {
        ByteRing_PushFromIsr(&ring, (uint8_t)(100U + i));
    }

    for (i = 8U; i < 15U; ++i)
    {
        EXPECT_EQ_UINT(ByteRing_Pop(&ring, &value), 1U,
                       "old entries remain after head wrap");
        EXPECT_EQ_UINT(value, i,
                       "old entries retain order across wrap");
    }
    for (i = 0U; i < 8U; ++i)
    {
        EXPECT_EQ_UINT(ByteRing_Pop(&ring, &value), 1U,
                       "new entries pop after head wrap");
        EXPECT_EQ_UINT(value, (uint8_t)(100U + i),
                       "new wrapped entries retain FIFO order");
    }
    EXPECT_EQ_UINT(ByteRing_Pop(&ring, &value), 0U,
                   "wrapped ring is empty at the end");
}

static void test_timebase_invalid_and_normal_deadlines(void)
{
    uint32_t deadline = 100U;

    EXPECT_EQ_UINT(Timebase_PeriodicDue(100U, 0, 10U), 0U,
                   "null deadline is rejected");
    EXPECT_EQ_UINT(Timebase_PeriodicDue(100U, &deadline, 0U), 0U,
                   "zero period is rejected");
    EXPECT_EQ_UINT(Timebase_PeriodicDue(99U, &deadline, 10U), 0U,
                   "time before deadline is not due");
    EXPECT_EQ_UINT(deadline, 100U,
                   "early check does not modify deadline");
    EXPECT_EQ_UINT(Timebase_PeriodicDue(100U, &deadline, 10U), 1U,
                   "time equal to deadline is due");
    EXPECT_EQ_UINT(deadline, 110U,
                   "due check advances by exactly one period");
    EXPECT_EQ_UINT(Timebase_PeriodicDue(135U, &deadline, 10U), 1U,
                   "late task remains due");
    EXPECT_EQ_UINT(deadline, 120U,
                   "late task advances one period per scheduler call");
}

static void test_timebase_uint32_wraparound(void)
{
    uint32_t deadline = 0xFFFFFFF0UL;

    EXPECT_EQ_UINT(Timebase_PeriodicDue(0xFFFFFFE0UL, &deadline, 20U), 0U,
                   "time before a pre-wrap deadline is not due");
    EXPECT_EQ_UINT(deadline, 0xFFFFFFF0UL,
                   "pre-wrap early check preserves deadline");

    EXPECT_EQ_UINT(Timebase_PeriodicDue(0x00000004UL, &deadline, 20U), 1U,
                   "time after uint32 wrap reaches deadline");
    EXPECT_EQ_UINT(deadline, 0x00000004UL,
                   "deadline addition wraps safely");

    EXPECT_EQ_UINT(Timebase_PeriodicDue(0x00000003UL, &deadline, 20U), 0U,
                   "time before the wrapped deadline is not due");
    EXPECT_EQ_UINT(Timebase_PeriodicDue(0x00000004UL, &deadline, 20U), 1U,
                   "wrapped deadline is due at equality");
    EXPECT_EQ_UINT(deadline, 0x00000018UL,
                   "post-wrap deadline continues normally");
}

int main(void)
{
    test_ring_null_and_empty_inputs();
    test_ring_capacity_overflow_and_order();
    test_ring_index_wraparound();
    test_timebase_invalid_and_normal_deadlines();
    test_timebase_uint32_wraparound();

    printf("boundary_tests: %u assertions, %u failures\n",
           g_tests, g_failures);
    return (g_failures == 0U) ? 0 : 1;
}
