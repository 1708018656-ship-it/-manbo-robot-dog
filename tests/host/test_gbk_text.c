/* Host tests for GbkText_SafeLength (SYN6288 GBK-safe truncation). */
#include <stdio.h>
#include <string.h>
#include "GbkText.h"

static int failures = 0, assertions = 0;
#define CHECK_EQ(a, b) do { assertions++; if ((size_t)(a) != (size_t)(b)) { \
    failures++; printf("FAIL %s:%d %s=%zu expected %zu\n", __FILE__, __LINE__, #a, (size_t)(a), (size_t)(b)); } } while (0)

int main(void)
{
    /* "[v12]" + GBK 我爱你 (6 bytes) */
    const char *mixed = "[v12]\xCE\xD2\xB0\xAE\xC4\xE3";
    char long_text[400];
    size_t i;

    CHECK_EQ(GbkText_SafeLength(NULL, 10), 0);
    CHECK_EQ(GbkText_SafeLength("", 10), 0);
    CHECK_EQ(GbkText_SafeLength("abc", 10), 3);
    CHECK_EQ(GbkText_SafeLength("abcdef", 4), 4);
    CHECK_EQ(GbkText_SafeLength(mixed, 100), 11);
    CHECK_EQ(GbkText_SafeLength(mixed, 11), 11);
    CHECK_EQ(GbkText_SafeLength(mixed, 10), 9);  /* would split 3rd char */
    CHECK_EQ(GbkText_SafeLength(mixed, 8), 7);   /* would split 2nd char */
    CHECK_EQ(GbkText_SafeLength(mixed, 6), 5);
    CHECK_EQ(GbkText_SafeLength("ab\xCE", 10), 2); /* dangling lead byte */

    /* 199 ASCII + one GBK char must not exceed the 200-byte module limit */
    for (i = 0; i < 199; ++i) long_text[i] = 'a';
    long_text[199] = (char)0xCE; long_text[200] = (char)0xD2; long_text[201] = '\0';
    CHECK_EQ(GbkText_SafeLength(long_text, SYN6288_MAX_TEXT_BYTES), 199);

    /* Longest prompt in Mode.c (13-byte prefix + 21 GBK chars = 55 bytes)
       used to be cut at 44 bytes; it must now fit completely. */
    CHECK_EQ(GbkText_SafeLength("[v12][m0][t5]\xC4\xE3\xC4\xE3\xC4\xE3\xC4\xE3\xC4\xE3\xC4\xE3\xC4\xE3"
                                "\xC4\xE3\xC4\xE3\xC4\xE3\xC4\xE3\xC4\xE3\xC4\xE3\xC4\xE3\xC4\xE3\xC4\xE3"
                                "\xC4\xE3\xC4\xE3\xC4\xE3\xC4\xE3\xC4\xE3", SYN6288_MAX_TEXT_BYTES), 55);
    CHECK_EQ(SYN6288_FRAME_CAPACITY, 206);

    printf("gbk_text_tests: %d assertions, %d failures\n", assertions, failures);
    return failures != 0;
}
